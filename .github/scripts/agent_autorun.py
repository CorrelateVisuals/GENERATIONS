#!/usr/bin/env python3
from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
import re
import subprocess
import time
from pathlib import Path
from typing import Any, Dict, List, Tuple
from urllib import request

ROOT = Path(os.environ.get("GITHUB_WORKSPACE", ".")).resolve()
AGENTS_DIR = ROOT / ".github" / "agents"
PARTY_DIR = AGENTS_DIR / "party"
TOWN_DIR = AGENTS_DIR / "town"
GUILDS_DIR = AGENTS_DIR / "guilds"
RUNS_DIR = AGENTS_DIR / "runs"
CACHE_DIR = RUNS_DIR / "cache"
PROPOSALS_DIR = AGENTS_DIR / "proposals"
METRICS_PATH = TOWN_DIR / "metrics.jsonl"
MACROS_JSON = TOWN_DIR / "macros.json"

FORCE_RERUN = os.environ.get("FORCE_RERUN", "false").strip().lower() == "true"

DEFAULT_OPENAI_URL = "https://api.openai.com/v1/chat/completions"
DEFAULT_GITHUB_URL = "https://api.githubcopilot.com/chat/completions"

LLM_PROVIDER = os.environ.get("LLM_PROVIDER", "auto").strip().lower()
LLM_API_URL = os.environ.get("LLM_API_URL", "").strip()
LLM_MODEL = os.environ.get("LLM_MODEL", "").strip()
LLM_API_KEY = os.environ.get("LLM_API_KEY", "").strip()
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN", "").strip()

TASK_MODE = os.environ.get("TASK_MODE", "main").strip().lower()  # main | self
AGENT_ONLY = os.environ.get("AGENT_ONLY", "").strip()
AGENT_SET = os.environ.get("AGENT_SET", "").strip()
TASK_COMMAND = os.environ.get("TASK_COMMAND", "").strip()
MACRO_MODE = os.environ.get("MACRO_MODE", "").strip().lower()
AUTO_APPLY_PATCH = os.environ.get("AUTO_APPLY_PATCH", "false").strip().lower() == "true"
MAX_PATCH_FILES = int(os.environ.get("MAX_PATCH_FILES", "8"))
MAX_PATCH_LINES = int(os.environ.get("MAX_PATCH_LINES", "400"))
GUARD_BUILD_CMD = os.environ.get("GUARD_BUILD_CMD", "").strip()

if LLM_PROVIDER == "auto":
    # Explicitly check for LLM_API_KEY first (non-empty string)
    # Only fall back to GITHUB_TOKEN if LLM_API_KEY is not set
    if LLM_API_KEY:
        LLM_PROVIDER = "openai"
    elif GITHUB_TOKEN:
        LLM_PROVIDER = "github"
    else:
        raise SystemExit("Missing credentials. Set LLM_API_KEY or GITHUB_TOKEN.")

if LLM_PROVIDER == "openai":
    if not LLM_API_URL:
        LLM_API_URL = DEFAULT_OPENAI_URL
    if not LLM_MODEL:
        LLM_MODEL = "gpt-4o-mini"
    AUTH_TOKEN = LLM_API_KEY
elif LLM_PROVIDER == "github":
    if not LLM_API_URL:
        LLM_API_URL = DEFAULT_GITHUB_URL
    if not LLM_MODEL:
        raise SystemExit("Missing LLM_MODEL for GitHub Models.")
    AUTH_TOKEN = GITHUB_TOKEN
else:
    raise SystemExit(f"Unknown LLM_PROVIDER '{LLM_PROVIDER}'.")

if not AUTH_TOKEN:
    raise SystemExit("Missing credentials. Set LLM_API_KEY or GITHUB_TOKEN.")


# ---------------------------------------------------------------------------
# Macro schema — single source of truth loaded from macros.json
# ---------------------------------------------------------------------------

def load_macro_schema() -> Dict[str, Any]:
    """Load macro definitions from macros.json. Returns empty dict on missing file."""
    if not MACROS_JSON.exists():
        print(f"WARNING: {MACROS_JSON} not found, macros disabled")
        return {}
    raw = MACROS_JSON.read_text(encoding="utf-8")
    schema = json.loads(raw)
    return schema


MACRO_SCHEMA = load_macro_schema()
MACRO_DEFS: Dict[str, Any] = MACRO_SCHEMA.get("macros", {})
RETRY_CONFIG = MACRO_SCHEMA.get("retry", {"max_attempts": 3, "backoff_seconds": [2, 8, 30]})
VALIDATION_CONFIG = MACRO_SCHEMA.get("validation", {"min_output_length": 100, "required_section_count": 4})
REQUIRED_SECTIONS = MACRO_SCHEMA.get("required_output_sections", [])
GATE_CONFIG = MACRO_SCHEMA.get("quality_gates", {
    "dissent_threshold": 0.5,
    "max_output_chars": 4000,
    "halt_on_low_confidence": True,
    "validate_file_refs": True,
})


def resolve_macro(macro_name: str) -> Dict[str, Any]:
    """Look up a macro by name (case-insensitive). Returns empty dict if not found."""
    for key, definition in MACRO_DEFS.items():
        if key.lower() == macro_name.lower():
            return definition
    return {}


def get_macro_agents(macro_name: str) -> List[str]:
    """Return the agent list for a macro from the schema."""
    defn = resolve_macro(macro_name)
    return defn.get("agents", [])


def get_macro_directive(macro_name: str, agent_name: str) -> str:
    """Return the directive text for a specific agent within a macro."""
    defn = resolve_macro(macro_name)
    return defn.get("directives", {}).get(agent_name, "")


def is_independent_macro(macro_name: str) -> bool:
    """Check if a macro runs agents independently (no shared context)."""
    defn = resolve_macro(macro_name)
    return defn.get("mode", "sequential") == "independent"


def get_macro_knob(macro_name: str, knob: str, default: Any = None) -> Any:
    """Read a per-macro override knob, falling back to global config or default."""
    defn = resolve_macro(macro_name)
    if knob in defn:
        return defn[knob]
    # Fall back to global sections
    global_map = {
        "max_output_chars": GATE_CONFIG.get("max_output_chars", 4000),
        "required_sections": REQUIRED_SECTIONS,
        "cross_confirm": True,
        "context_chars": int(os.environ.get("MAX_CODE_CONTEXT", "12000")),
        "handoff_chars": 1500,
        "retry_on_gate_fail": True,
    }
    return global_map.get(knob, default)


# ---------------------------------------------------------------------------
# Quality Gates — Factory-grade output validation
# ---------------------------------------------------------------------------

def _count_sections(output: str) -> int:
    """Count how many required output sections are present."""
    return _count_sections_list(output, REQUIRED_SECTIONS)


def _count_sections_list(output: str, sections: List[str]) -> int:
    """Count how many sections from a specific list are present.
    Handles both new '## TODOs' format and legacy '1) Main Task Outcome' format."""
    found = 0
    for section in sections:
        if section.startswith("## "):
            # New heading format: match '## TODOs' or '## Code Proposal' etc.
            heading = section[3:].strip()
            pattern = rf"##\s*{re.escape(heading)}"
        elif ")" in section:
            # Legacy numbered format: '1) Main Task Outcome'
            label = section.split(")", 1)[1].strip()
            pattern = rf"\d+\)\s*{re.escape(label)}"
        else:
            pattern = re.escape(section)
        if re.search(pattern, output, re.IGNORECASE):
            found += 1
    return found


def _count_dissent_markers(output: str) -> Tuple[int, int, int]:
    """Return (concur_count, qualify_count, dissent_count) from cross-confirmation markers."""
    concur = len(re.findall(r"\u2705\s*CONCUR|CONCUR", output))
    qualify = len(re.findall(r"\u26a0\ufe0f\s*QUALIFY|QUALIFY", output))
    dissent = len(re.findall(r"\u274c\s*DISSENT|DISSENT", output))
    return concur, qualify, dissent


def _extract_confidence(output: str) -> str:
    """Extract Combined confidence marker from output."""
    m = re.search(r"Combined confidence:\s*(HIGH|MEDIUM|LOW)", output, re.IGNORECASE)
    return m.group(1).upper() if m else "UNKNOWN"


def _validate_file_refs(output: str) -> List[str]:
    """Check that file paths referenced in code proposals actually exist."""
    bad = []
    search_dirs = [ROOT, ROOT / "src", ROOT / "shaders"]
    for m in re.finditer(r"(?:^|[\s`])([a-zA-Z]\S+\.(?:cpp|h|hpp|comp|frag|vert|glsl|tesc|tese))(?:[\s`]|$)", output, re.MULTILINE):
        ref = m.group(1)
        basename = Path(ref).name
        if any((d / ref).exists() or (d / basename).exists() for d in search_dirs):
            continue
        bad.append(ref)
    return bad


# ---------------------------------------------------------------------------
# Reasoning / public split
# ---------------------------------------------------------------------------

def strip_reasoning(raw: str) -> Tuple[str, str]:
    """Split agent output into public (PR-facing) and reasoning (log-only).
    Agents wrap analysis in <reasoning>...</reasoning> tags.
    Returns (public_text, reasoning_text)."""
    reasoning_parts: List[str] = []
    def _collect(m: re.Match) -> str:
        reasoning_parts.append(m.group(1).strip())
        return ""
    public = re.sub(r"<reasoning>(.*?)</reasoning>", _collect, raw, flags=re.DOTALL).strip()
    # Collapse multiple blank lines left after stripping
    public = re.sub(r"\n{3,}", "\n\n", public)
    return public, "\n\n".join(reasoning_parts)


def extract_one_liner(output: str) -> str:
    """Extract the first meaningful sentence from an agent's output for use in summary."""
    # Look for Main Task Outcome or first paragraph after heading
    m = re.search(r"(?:Main Task Outcome|## Summary)\s*\n+(.*?)(?:\n\n|\n##|$)", output, re.IGNORECASE)
    if m:
        line = m.group(1).strip().split("\n")[0]
        return line[:200]
    # Fallback: first non-empty, non-heading line
    for line in output.split("\n"):
        line = line.strip()
        if line and not line.startswith("#") and not line.startswith("```") and len(line) > 20:
            return line[:200]
    return "(no summary available)"


def aggregate_todos(outputs: Dict[str, str]) -> str:
    """Collect all TODO items from all agents into one deduplicated list."""
    todos: List[str] = []
    seen: set = set()
    for name, text in outputs.items():
        # Find TODO section
        m = re.search(r"(?:Actionable\s+)?TODOs?\s*\n(.*?)(?:\n## |\n\d+\)|$)", text, flags=re.IGNORECASE | re.DOTALL)
        if not m:
            continue
        for line in m.group(1).strip().split("\n"):
            line = line.strip()
            if line.startswith("- [") or line.startswith("- `"):
                # Normalise for dedup
                key = re.sub(r"\s+", " ", line.lower())
                if key not in seen:
                    seen.add(key)
                    todos.append(line)
    if not todos:
        return "No actionable TODOs extracted."
    return "\n".join(todos)


def run_quality_gate(
    agent_name: str,
    output: str,
    macro_mode: str,
) -> Tuple[str, str, Dict[str, Any]]:
    """Run quality gates on agent output.
    Returns (action, message, metrics_extras) where action is PASS/RETRY/HALT/WARN."""
    extras: Dict[str, Any] = {}
    issues: List[str] = []

    # Per-macro section requirements (override global)
    macro_sections = get_macro_knob(macro_mode, "required_sections", REQUIRED_SECTIONS)
    macro_required_count = len(macro_sections) if macro_sections else VALIDATION_CONFIG.get("required_section_count", 4)

    # Gate 1: Structure — required sections present?
    min_len = VALIDATION_CONFIG.get("min_output_length", 100)
    sections_found = _count_sections_list(output, macro_sections) if macro_sections else _count_sections(output)
    extras["sections_found"] = sections_found
    extras["required_count"] = macro_required_count

    if len(output.strip()) < min_len:
        return "RETRY", f"Output too short ({len(output.strip())} chars, minimum {min_len}). Rewrite with all required sections.", extras

    if sections_found < macro_required_count:
        issues.append(f"Only {sections_found}/{macro_required_count} required sections")
        section_names = ', '.join(macro_sections) if macro_sections else ', '.join(REQUIRED_SECTIONS)
        return "RETRY", f"Missing sections: {sections_found}/{macro_required_count} found. Include: {section_names}", extras

    # Gate 2: DISSENT threshold (only for validator agents)
    concur, qualify, dissent = _count_dissent_markers(output)
    extras["concur_count"] = concur
    extras["qualify_count"] = qualify
    extras["dissent_count"] = dissent
    total_markers = concur + qualify + dissent
    dissent_threshold = GATE_CONFIG.get("dissent_threshold", 0.5)
    if total_markers > 0 and dissent / total_markers > dissent_threshold:
        confidence = _extract_confidence(output)
        extras["confidence"] = confidence
        return "HALT", f"High dissent: {dissent}/{total_markers} findings marked DISSENT (threshold {dissent_threshold:.0%}). Halting pipeline.", extras

    # Gate 3: Confidence floor
    confidence = _extract_confidence(output)
    extras["confidence"] = confidence
    if GATE_CONFIG.get("halt_on_low_confidence", True) and confidence == "LOW":
        return "HALT", f"Combined confidence is LOW. Halting pipeline — not safe to continue.", extras

    # Gate 4: File reference validation
    if GATE_CONFIG.get("validate_file_refs", True):
        bad_refs = _validate_file_refs(output)
        if bad_refs:
            extras["invalid_file_refs"] = bad_refs
            issues.append(f"References non-existent files: {', '.join(bad_refs[:5])}")

    # Gate 5: Output truncation (per-macro cap)
    max_chars = get_macro_knob(macro_mode, "max_output_chars", 4000)
    extras["output_truncated"] = len(output) > max_chars
    # Note: we don't truncate here — caller handles it. Just flag it.

    if issues:
        return "WARN", "; ".join(issues), extras

    return "PASS", "All gates passed.", extras


# ---------------------------------------------------------------------------
# Metrics — JSONL append for throughput and defect tracking
# ---------------------------------------------------------------------------

def append_metric(record: Dict[str, Any]) -> None:
    """Append a single JSON record to metrics.jsonl."""
    METRICS_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(METRICS_PATH, "a", encoding="utf-8") as f:
        f.write(json.dumps(record, default=str) + "\n")


def load_trailing_metrics(days: int = 7) -> List[Dict[str, Any]]:
    """Load metrics from the last N days for dashboard summaries."""
    if not METRICS_PATH.exists():
        return []
    cutoff = (dt.datetime.utcnow() - dt.timedelta(days=days)).isoformat()
    records = []
    for line in METRICS_PATH.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            rec = json.loads(line)
            if rec.get("timestamp", "") >= cutoff:
                records.append(rec)
        except json.JSONDecodeError:
            continue
    return records


def compute_metrics_summary(trailing: List[Dict[str, Any]]) -> str:
    """Compute factory dashboard summary from trailing metrics."""
    if not trailing:
        return "No trailing metrics available yet."
    total = len(trailing)
    gate_results = [r.get("gate_result", "UNKNOWN") for r in trailing]
    pass_count = gate_results.count("PASS")
    retry_count = gate_results.count("RETRY")
    halt_count = gate_results.count("HALT")
    warn_count = gate_results.count("WARN")
    cache_hits = sum(1 for r in trailing if r.get("cache_hit", False))
    avg_latency = sum(r.get("latency_ms", 0) for r in trailing) / max(total, 1)

    # Per-agent reliability
    agent_stats: Dict[str, Dict[str, int]] = {}
    for r in trailing:
        agent = r.get("agent", "unknown")
        if agent not in agent_stats:
            agent_stats[agent] = {"total": 0, "pass": 0}
        agent_stats[agent]["total"] += 1
        if r.get("gate_result") == "PASS":
            agent_stats[agent]["pass"] += 1

    lines = [
        "## Factory Metrics (trailing 7 days)",
        "",
        f"| Metric | Value |",
        f"|--------|-------|",
        f"| Total agent calls | {total} |",
        f"| Pass rate | {pass_count}/{total} ({100*pass_count//max(total,1)}%) |",
        f"| Retry rate | {retry_count}/{total} |",
        f"| Halt rate | {halt_count}/{total} |",
        f"| Warn rate | {warn_count}/{total} |",
        f"| Cache hit rate | {cache_hits}/{total} ({100*cache_hits//max(total,1)}%) |",
        f"| Avg latency | {avg_latency:.0f}ms |",
        "",
        "### Agent Reliability",
        "",
        "| Agent | Pass Rate |",
        "|-------|----------|",
    ]
    for agent, stats in sorted(agent_stats.items()):
        pct = 100 * stats["pass"] // max(stats["total"], 1)
        lines.append(f"| {agent} | {stats['pass']}/{stats['total']} ({pct}%) |")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Determinism — fingerprint, cache, macro-specific temperature
# ---------------------------------------------------------------------------

def compute_fingerprint(task_md: str, scoped_code: str, agent_name: str, macro_mode: str) -> str:
    """SHA-256 fingerprint of inputs that determine agent output."""
    blob = f"{task_md}|{scoped_code}|{agent_name}|{macro_mode}"
    return hashlib.sha256(blob.encode("utf-8")).hexdigest()[:16]


def cache_lookup(fingerprint: str) -> str | None:
    """Return cached result if fingerprint exists, else None."""
    if FORCE_RERUN:
        return None
    cache_file = CACHE_DIR / f"{fingerprint}.md"
    if cache_file.exists():
        return cache_file.read_text(encoding="utf-8")
    return None


def cache_store(fingerprint: str, result: str) -> None:
    """Store result in cache keyed by fingerprint."""
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    cache_file = CACHE_DIR / f"{fingerprint}.md"
    cache_file.write_text(result, encoding="utf-8")


def get_temperature(macro_mode: str) -> float:
    """Execution macros (Charge, Follow) use temp 0.0 for determinism.
    Exploration macros (Think) keep 0.2 for creative variance."""
    defn = resolve_macro(macro_mode)
    return defn.get("temperature", 0.2)


def read_text(path: Path, default: str = "") -> str:
    if not path.exists():
        return default
    return path.read_text(encoding="utf-8")


def read_capped(path: Path, max_chars: int = 3000) -> str:
    content = read_text(path)
    if len(content) <= max_chars:
        return content
    head = content[: max_chars // 2]
    tail = content[-max_chars // 2 :]
    return f"{head}\n\n... [truncated] ...\n\n{tail}"


AGENT_GUILDS: Dict[str, List[str]] = {
    "C++ Lead": ["performance.md", "architecture.md"],
    "Vulkan Guru": ["performance.md", "gpu-pipeline.md"],
    "Kernel Expert": ["performance.md", "gpu-pipeline.md"],
    "Refactorer": ["architecture.md"],
    "Guild Master": [],
}


def load_guild_context(agent_name: str) -> str:
    guild_files = AGENT_GUILDS.get(agent_name, [])
    if not guild_files:
        return ""
    parts = []
    for gf in guild_files:
        path = GUILDS_DIR / gf
        content = read_text(path)
        if content:
            parts.append(content)
    procedures = read_text(TOWN_DIR / "procedures.md")
    if procedures.strip():
        parts.append(procedures)
    return "\n\n".join(parts) if parts else ""


GOVERNANCE_LOG = GUILDS_DIR / "governance-log.md"


def build_guild_master_context() -> str:
    """Build specialized context for Guild Master: metrics, agent profiles, guild policies, run history."""
    parts = [
        "# Guild Master Context",
        "",
        "You are the Guild Master — the ONLY agent with authority to observe and set guild policies.",
        "You do NOT analyze source code. You govern the agent pipeline.",
        "",
    ]

    # 1. Factory metrics (full observation)
    trailing = load_trailing_metrics(days=14)
    if trailing:
        parts.append(compute_metrics_summary(trailing))
        parts.append("")
        parts.append("### Recent Metric Records (last 30)")
        parts.append("```json")
        for rec in trailing[-30:]:
            parts.append(json.dumps(rec, default=str))
        parts.append("```")
    else:
        parts.append("*No metrics data available yet. This is the first Guild Master run.*")
    parts.append("")

    # 2. Agent profiles (full — the GM needs to judge alignment)
    parts.append("## Current Class Profiles")
    for agent_file in sorted(PARTY_DIR.glob("*.md")):
        content = read_text(agent_file)
        parts.append(f"### {agent_file.stem}")
        parts.append(content[:1200] if len(content) > 1200 else content)
        parts.append("")

    # 3. Guild membership map
    parts.append("## Guild Membership Map")
    for agent_name, guild_files in AGENT_GUILDS.items():
        if agent_name == "Guild Master":
            continue
        guilds_str = ", ".join(guild_files) if guild_files else "(no guild)"
        parts.append(f"- **{agent_name}** → {guilds_str}")
    parts.append("")

    # 4. Guild policies (FULL content — the GM sets these, needs to see everything)
    parts.append("## Current Guild Policies")
    for guild_file in sorted(GUILDS_DIR.glob("*.md")):
        if guild_file.name in ("guild-master.md", "governance-log.md"):
            continue
        content = read_text(guild_file)
        parts.append(f"### {guild_file.stem}")
        parts.append(content)
        parts.append("")

    # 5. Procedures (full)
    procedures = read_text(TOWN_DIR / "procedures.md")
    if procedures.strip():
        parts.append("## Current Procedures")
        parts.append(procedures[:3000] if len(procedures) > 3000 else procedures)
    parts.append("")

    # 6. Quality gate config (so GM can propose threshold changes with awareness)
    gate_cfg = GATE_CONFIG
    parts.append("## Current Quality Gate Config")
    parts.append(f"- Dissent threshold: {gate_cfg.get('dissent_threshold', 0.5)}")
    parts.append(f"- Max output chars: {gate_cfg.get('max_output_chars', 4000)}")
    parts.append(f"- Halt on low confidence: {gate_cfg.get('halt_on_low_confidence', True)}")
    parts.append(f"- Validate file refs: {gate_cfg.get('validate_file_refs', True)}")
    parts.append("")

    # 7. Governance history (previous decisions)
    if GOVERNANCE_LOG.exists():
        gov_log = read_text(GOVERNANCE_LOG)
        parts.append("## Previous Governance Decisions")
        # Show last 3000 chars of the log
        parts.append(gov_log[-3000:] if len(gov_log) > 3000 else gov_log)
    else:
        parts.append("## Previous Governance Decisions")
        parts.append("*No governance log yet. This is the first Guild Master assessment.*")
    parts.append("")

    # 8. Recent run reports (so GM sees what agents actually produced)
    if RUNS_DIR.exists():
        run_files = sorted(RUNS_DIR.glob("*.md"), reverse=True)[:3]
        if run_files:
            parts.append("## Recent Run Reports (last 3)")
            for rf in run_files:
                content = read_text(rf)
                parts.append(f"### {rf.name}")
                parts.append(content[:2000] if len(content) > 2000 else content)
                parts.append("")

    return "\n".join(parts)


def extract_guild_master_actions(output: str) -> Dict[str, List[str]]:
    """Parse CLASS-CHANGE and GUILD-POLICY directives from Guild Master output."""
    actions: Dict[str, List[str]] = {"class_changes": [], "guild_policies": []}
    for line in output.split("\n"):
        stripped = line.strip()
        if stripped.startswith("CLASS-CHANGE:"):
            actions["class_changes"].append(stripped)
        elif stripped.startswith("GUILD-POLICY:"):
            actions["guild_policies"].append(stripped)
    return actions


def apply_guild_master_actions(actions: Dict[str, List[str]], run_id: str) -> str:
    """Write Guild Master decisions to the governance log. Returns summary of what was applied."""
    if not actions["class_changes"] and not actions["guild_policies"]:
        return "No governance actions to apply."

    today = dt.date.today().isoformat()
    entries = []

    entries.append(f"\n## Assessment — {run_id} ({today})\n")

    if actions["class_changes"]:
        entries.append("### Class Change Directives")
        entries.append("| # | Directive | Status |")
        entries.append("|---|---|---|")
        for i, cc in enumerate(actions["class_changes"], 1):
            entries.append(f"| {i} | {cc} | LOGGED |")
        entries.append("")

    if actions["guild_policies"]:
        entries.append("### Guild Policy Directives")
        entries.append("| # | Directive | Status |")
        entries.append("|---|---|---|")
        for i, gp in enumerate(actions["guild_policies"], 1):
            entries.append(f"| {i} | {gp} | LOGGED |")
        entries.append("")

        # Append policy items to procedures.md for visibility
        procedures_path = TOWN_DIR / "procedures.md"
        with open(procedures_path, "a", encoding="utf-8") as f:
            f.write(f"\n### Guild Master Policy ({today}, {run_id})\n")
            for gp in actions["guild_policies"]:
                f.write(f"- {gp}\n")

    log_text = "\n".join(entries)

    # Create or append to governance log
    if GOVERNANCE_LOG.exists():
        with open(GOVERNANCE_LOG, "a", encoding="utf-8") as f:
            f.write(log_text)
    else:
        header = "# Governance Log\n\nDecisions and directives issued by the Guild Master.\n"
        GOVERNANCE_LOG.write_text(header + log_text, encoding="utf-8")

    n_cc = len(actions["class_changes"])
    n_gp = len(actions["guild_policies"])
    return f"Governance log updated: {n_cc} class-change(s), {n_gp} guild-policy(ies) recorded."


# ---------------------------------------------------------------------------
# Guild Review — lightweight post-sequence quality assessment
# ---------------------------------------------------------------------------

def run_guild_review(task_md: str, outputs: Dict[str, str],
                     scoped_code: str, macro_mode: str | None) -> Dict[str, str]:
    """Run a lightweight Guild Master review of agent outputs before patch generation.

    Returns a dict with keys:
      - verdict: APPROVE | CAUTION | BLOCK
      - summary: one-paragraph assessment
      - hallucination_flags: comma-separated list of suspect API calls (may be empty)
      - raw: full review text
    """
    agent_summaries = []
    for name, text in outputs.items():
        if text.startswith("*Skipped"):
            continue
        # Take first 1500 chars of each agent's output for the review
        excerpt = text[:1500]
        agent_summaries.append(f"### {name}\n{excerpt}")
    if not agent_summaries:
        return {"verdict": "APPROVE", "summary": "No agent outputs to review.", "hallucination_flags": "", "raw": ""}

    # Build the API inventory from scoped code for grounding check
    api_inventory = _extract_api_inventory(scoped_code)

    review_prompt = f"""You are the Guild Master performing a post-sequence quality review.

# Task
{task_md[:800]}

# Known APIs (from actual source files)
{api_inventory}

# Agent Outputs (excerpts)
{"".join(agent_summaries)}

# Your Review Instructions
1. Check each agent's Code Proposal for **hallucinated APIs** — function/method calls that do NOT appear in the Known APIs list above. List any suspect calls.
2. Check for **contradictions** between agents (e.g., one says add try/catch, another says remove it).
3. Check for **scope violations** — changes to files not mentioned in the task.
4. Assess overall **coherence** — do the proposals work together toward the task goal?

# Required Output Format (exactly this structure)
VERDICT: <APPROVE|CAUTION|BLOCK>
SUMMARY: <one paragraph, max 3 sentences>
HALLUCINATION_FLAGS: <comma-separated list of suspect API calls, or "none">
NOTES: <any additional observations, max 2 sentences>
"""

    try:
        temperature = get_temperature("Guild")
        result = call_llm(review_prompt, temperature=temperature)
    except Exception as e:
        print(f"  ⚠ Guild review LLM call failed: {e}")
        return {"verdict": "APPROVE", "summary": f"Review skipped (error: {e})", "hallucination_flags": "", "raw": ""}

    # Parse structured output
    verdict = "APPROVE"
    summary = ""
    halluc_flags = ""
    for line in result.split("\n"):
        line_s = line.strip()
        if line_s.startswith("VERDICT:"):
            v = line_s.split(":", 1)[1].strip().upper()
            if v in ("APPROVE", "CAUTION", "BLOCK"):
                verdict = v
        elif line_s.startswith("SUMMARY:"):
            summary = line_s.split(":", 1)[1].strip()
        elif line_s.startswith("HALLUCINATION_FLAGS:"):
            halluc_flags = line_s.split(":", 1)[1].strip()
            if halluc_flags.lower() == "none":
                halluc_flags = ""

    return {"verdict": verdict, "summary": summary, "hallucination_flags": halluc_flags, "raw": result}


MAX_PROMPT_CHARS = int(os.environ.get("MAX_PROMPT_CHARS", "24000"))


def _trim_prompt(prompt: str, budget: int = MAX_PROMPT_CHARS) -> str:
    """Trim an oversized prompt by cutting scoped code and previous outputs."""
    if len(prompt) <= budget:
        return prompt
    excess = len(prompt) - budget
    # Strategy: shorten the biggest sections first.
    # 1) Cut '# Relevant Code Context' section
    code_header = "# Relevant Code Context"
    idx = prompt.find(code_header)
    if idx != -1:
        next_header = prompt.find("\n# ", idx + len(code_header))
        if next_header == -1:
            next_header = len(prompt)
        code_section = prompt[idx:next_header]
        if len(code_section) > 2000:
            keep = max(2000, len(code_section) - excess)
            trimmed_section = code_section[:keep] + "\n... [auto-trimmed to fit payload budget] ...\n"
            prompt = prompt[:idx] + trimmed_section + prompt[next_header:]
            if len(prompt) <= budget:
                return prompt
    # 2) Cut '# Previous Agent Outputs' section
    prev_header = "# Previous Agent Outputs"
    idx = prompt.find(prev_header)
    if idx != -1:
        next_header = prompt.find("\n# ", idx + len(prev_header))
        if next_header == -1:
            next_header = len(prompt)
        prev_section = prompt[idx:next_header]
        if len(prev_section) > 1000:
            keep = max(1000, len(prev_section) - (len(prompt) - budget))
            trimmed_section = prev_section[:keep] + "\n... [auto-trimmed] ...\n"
            prompt = prompt[:idx] + trimmed_section + prompt[next_header:]
    # 3) Last resort: hard truncate
    if len(prompt) > budget:
        suffix = "\n... [prompt hard-truncated to fit payload limit]"
        prompt = prompt[:budget - len(suffix)] + suffix
    return prompt


def _llm_request(prompt: str, temperature: float = 0.2) -> str:
    """Single LLM API call (no retry). Both providers use the same chat/completions format."""
    prompt = _trim_prompt(prompt)
    payload = {
        "model": LLM_MODEL,
        "messages": [{"role": "user", "content": prompt}],
        "temperature": temperature,
    }
    data = json.dumps(payload).encode("utf-8")
    req = request.Request(
        LLM_API_URL,
        data=data,
        headers={
            "Authorization": f"Bearer {AUTH_TOKEN}",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    with request.urlopen(req, timeout=180) as resp:
        raw = resp.read().decode("utf-8")
    body = json.loads(raw)

    if isinstance(body, dict):
        choices = body.get("choices", [])
        if choices:
            msg = choices[0].get("message", {})
            content = msg.get("content")
            if isinstance(content, str):
                return content
    return raw


def call_llm(prompt: str, temperature: float = 0.2) -> str:
    """LLM call with retry and exponential backoff from macros.json config."""
    max_attempts = RETRY_CONFIG.get("max_attempts", 3)
    backoff = RETRY_CONFIG.get("backoff_seconds", [2, 8, 30])

    last_error: Exception = RuntimeError("No attempts made")
    for attempt in range(max_attempts):
        try:
            result = _llm_request(prompt, temperature=temperature)
            if result and len(result.strip()) > 10:
                return result
            # Treat empty/trivial response as transient failure
            last_error = RuntimeError(f"Empty LLM response on attempt {attempt + 1}")
        except Exception as exc:
            last_error = exc
        wait = backoff[min(attempt, len(backoff) - 1)]
        print(f"  LLM attempt {attempt + 1}/{max_attempts} failed: {last_error}. Retrying in {wait}s...")
        time.sleep(wait)

    raise SystemExit(f"LLM call failed after {max_attempts} attempts: {last_error}")


def parse_scope_files(task_md: str) -> List[Path]:
    files: List[Path] = []
    for line in task_md.splitlines():
        line = line.strip()
        if not (line.startswith("-") and "`" in line):
            continue
        for match in re.findall(r"`([^`]+)`", line):
            if "*" in match:
                for hit in sorted(ROOT.glob(match)):
                    if hit.is_file():
                        files.append(hit)
            else:
                p = ROOT / match
                if p.exists() and p.is_file():
                    files.append(p)

    seen = set()
    unique: List[Path] = []
    for p in files:
        key = str(p.resolve())
        if key in seen:
            continue
        seen.add(key)
        unique.append(p)
    return unique


def extract_task_id(task_md: str) -> str:
    m = re.search(r"## Task ID\s*\n\s*([A-Za-z0-9_-]+)", task_md)
    return m.group(1) if m else "TASK"


def extract_next_run(outputs: Dict[str, str], full_next_runs: Dict[str, str] | None = None) -> str:
    """Extract and consolidate next-run recommendations from all agent outputs.
    Uses pre-extracted full_next_runs (captured before truncation) when available.
    Returns a ready-to-use next-task.md or empty string if no recommendations."""
    recs: List[str] = []
    for name, text in outputs.items():
        # Prefer pre-extracted full next-run (captured before output truncation)
        if full_next_runs and name in full_next_runs:
            rec = full_next_runs[name]
            if rec and "none" not in rec.lower()[:20]:
                recs.append(f"- **{name}**: {rec}")
            continue
        # Fallback: extract from (possibly truncated) output
        m = re.search(
            r"(?:## Next Run|9\)\s*Recommended Next Run)([\s\S]*?)(?:\n## |\n\d+\)|\Z)",
            text,
            flags=re.IGNORECASE,
        )
        if m:
            rec = m.group(1).strip()
            if rec and "none" not in rec.lower()[:20]:
                recs.append(f"- **{name}**: {rec}")
    if not recs:
        return ""
    return "\n".join(recs)


def extract_todos_for(agent_name: str, text: str) -> str:
    pattern = rf"{re.escape(agent_name)}\s*TODOs?:([\s\S]*?)(?:\n\n|$)"
    m = re.search(pattern, text, flags=re.IGNORECASE)
    if not m:
        return "No explicit TODO block found; use previous agent findings as input."
    return m.group(1).strip()


MAX_CODE_CONTEXT = int(os.environ.get("MAX_CODE_CONTEXT", "12000"))


def _pair_headers(files: List[Path]) -> List[Path]:
    """Ensure .h/.cpp pairs stay together: if a .cpp is present, inject its .h right before it
    (and vice versa). This guarantees agents see class definitions alongside implementations."""
    present = {str(f.resolve()) for f in files}
    paired: List[Path] = []
    seen: set = set()
    for f in files:
        key = str(f.resolve())
        if key in seen:
            continue
        # If this is a .cpp, inject its .h first
        if f.suffix == ".cpp":
            header = f.with_suffix(".h")
            hkey = str(header.resolve())
            if header.exists() and hkey not in seen:
                paired.append(header)
                seen.add(hkey)
        # If this is a .h, also pull in its .cpp right after
        elif f.suffix == ".h":
            impl = f.with_suffix(".cpp")
            impl_key = str(impl.resolve())
            paired.append(f)
            seen.add(key)
            if impl.exists() and impl_key not in seen:
                paired.append(impl)
                seen.add(impl_key)
            continue
        paired.append(f)
        seen.add(key)
    return paired


def _sort_by_relevance(files: List[Path], task_keywords: List[str]) -> List[Path]:
    """Sort files so task-relevant files come first, consuming the context budget first."""
    def score(f: Path) -> int:
        name = f.stem.lower()
        s = 0
        for kw in task_keywords:
            if kw in name:
                s += 10
            # Also check parent dir name
            if kw in f.parent.name.lower():
                s += 5
        # Prefer smaller files (more likely to fit in budget)
        try:
            size = f.stat().st_size
            if size < 3000:
                s += 3  # small files = cheap, fit whole
            elif size < 6000:
                s += 1
        except OSError:
            pass
        # Headers slightly preferred (contain declarations = API surface)
        if f.suffix == ".h":
            s += 2
        return -s  # negative for ascending sort = highest score first
    return sorted(files, key=score)


def build_scoped_code(files: List[Path], max_chars: int = 12000,
                      task_keywords: List[str] | None = None,
                      per_file_cap: int = 3000) -> str:
    if not files:
        return "No in-scope files found."

    # Phase 1: Sort by task relevance (task-related files get budget priority)
    if task_keywords:
        files = _sort_by_relevance(files, task_keywords)

    # Phase 2: Pair headers with implementations
    files = _pair_headers(files)

    chunks = []
    total = 0
    for f in files:
        rel = f.relative_to(ROOT)
        snippet = read_capped(f, max_chars=per_file_cap)
        if total + len(snippet) > max_chars:
            remaining = max_chars - total
            if remaining > 200:
                snippet = snippet[:remaining] + "\n... [budget exceeded] ..."
            else:
                chunks.append(f"## {rel}\n... [omitted — context budget reached] ...")
                continue
        chunks.append(f"## {rel}\n{snippet}")
        total += len(snippet)
    return "\n\n".join(chunks)


def build_agent_prompt(
    base_md: str,
    profile_md: str,
    task_md: str,
    scoped_code: str,
    previous_outputs: Dict[str, str],
    incoming_todos: str,
    agent_name: str,
    guild_context: str = "",
    macro_directive: str = "",
    handoff_chars: int = 1500,
    cross_confirm: bool = True,
    macro_sections: List[str] | None = None,
) -> str:
    prev_parts = []
    for name, txt in previous_outputs.items():
        trimmed = txt[:handoff_chars] if len(txt) > handoff_chars else txt
        prev_parts.append(f"## {name} Output\n{trimmed}")
    prev = "\n\n".join(prev_parts)
    incoming_trimmed = incoming_todos[:handoff_chars] if len(incoming_todos) > handoff_chars else incoming_todos
    guild_section = ""
    if guild_context:
        guild_section = f"""
# Guild Context (shared procedures and vocabulary)
{guild_context[:3000]}
"""
    directive_section = ""
    if macro_directive:
        directive_section = f"""
# Macro Directive (FOLLOW THIS — it overrides default output format)
{macro_directive}
"""
    # Build section list — per-macro or global default
    sections_default = """## Output Structure Rules

Wrap ALL analysis, investigation notes, and reasoning in <reasoning> tags.
These are logged but NOT shown in the PR — keep them thorough but hidden.

Your VISIBLE output (shown in the PR) must contain ONLY these sections:

## TODOs
Actionable items. One per line, format:
- [ ] `path/file.cpp` L<line> — <specific change: what to add/remove/modify> [RISK: high/medium/low]

Be SPECIFIC:
- Name exact functions, variables, and line numbers from the provided code context.
- Write concrete actions: "add X before Y", "change A to B", "remove call to Z at line N".
- Do NOT write generic goals like "ensure X handles Y" or "validate that Z works".
- Every TODO must be independently actionable by someone who has never seen the code.

## Code Proposal
```cpp
// Only if this macro produces patches. Describe the INTENT of the change.
// Reference real functions from the Relevant Code Context where possible.
// The patch generator will verify all APIs exist — if you reference a method
// that doesn't exist, it will be skipped. Prefer wrapping/extending existing
// code over inventing new function signatures.
```

## Next Run
`<Macro> "<task_command>"` — one-line rationale.
Or: `None — task is self-contained.`

CRITICAL RULES:
- ONLY reference functions, types, methods, and variables that appear in the provided Relevant Code Context section.
- Do NOT invent or assume APIs that are not shown. If a function doesn't exist in the context, don't call it.
- Code proposals must compile against the existing codebase — no fictional interfaces.

Example complete output:

<reasoning>
Investigated swapchain recreation in Mechanics.cpp lines 45-80.
The current flow calls destroy() but does not wait for device idle.
This risks use-after-free on in-flight frames.
</reasoning>

## TODOs
- [ ] `src/vulkan_mechanics/Mechanics.cpp` L47 — add vkDeviceWaitIdle(device) call before destroy() in recreate() [RISK: high]
- [ ] `src/vulkan_mechanics/Mechanics.cpp` L52 — call create_framebuffers() after create() to rebuild framebuffers [RISK: high]

## Code Proposal
```cpp
void VulkanMechanics::BaseSwapchain::recreate() {
    vkDeviceWaitIdle(device);
    destroy();
    create();
}
```

## Next Run
`Follow "implement swapchain recreation safety"` — blast radius mapped, ready to execute."""

    if macro_sections:
        section_block = "## Output Structure Rules\n\nWrap ALL analysis, investigation notes, and reasoning in <reasoning> tags.\nThese are logged but NOT shown in the PR — keep them thorough but hidden.\n\nYour VISIBLE output (shown in the PR) must contain ONLY these sections:\n\n"
        for s in macro_sections:
            section_block += f"{s}\n"
        if cross_confirm:
            section_block += "\n## Cross-Confirmation\nFor each prior agent finding: CONCUR / QUALIFY / DISSENT with one-line evidence.\n"
    else:
        section_block = sections_default
        if cross_confirm:
            section_block += "\n\n## Cross-Confirmation\nFor each prior agent finding: CONCUR / QUALIFY / DISSENT with one-line evidence."

    return f"""
You are running inside the GENERATIONS multi-agent pipeline.

# Shared Base
{base_md}

# Agent Profile
{profile_md}
{guild_section}{directive_section}
# Active Task
{task_md}

# Incoming Handoff TODOs for {agent_name}
{incoming_trimmed}

# Relevant Code Context
{scoped_code}

# Previous Agent Outputs
{prev if prev else 'None'}

{section_block}
""".strip()


def build_self_task(schedule_md: str, readme_md: str) -> str:
    prompt = f"""
Create one autonomous in-between maintenance task for the GENERATIONS agent system.
Task must be unrelated to the current manual main task and safe to review in PR.

Use these docs:

# Agent README
{readme_md}

# Schedule
{schedule_md}

Return markdown with exact sections:
## Task ID
<SHORT-ID>

## Manual Action Statement
<one sentence>

## Scope
- In scope:
  - `<path>`
- Out of scope:
  - `<path/category>`

## Sequential Execution Order
1. C++ Lead
2. Vulkan Guru
3. Kernel Expert
4. Refactorer

## C++ Lead Required Handoff TODOs
- Vulkan Guru TODOs
- Kernel Expert TODOs
- Refactorer TODOs

## Output Location
- Run report: `.github/agents/runs/YYYY-MM-DD-choice-a.md`
- PR links: add under each agent section in the run report
""".strip()
    return call_llm(prompt)


def run_command(command: str) -> Tuple[int, str]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        shell=True,
        text=True,
        capture_output=True,
        check=False,
    )
    combined = (proc.stdout or "") + (proc.stderr or "")
    return proc.returncode, combined


def extract_patch(patch_text: str) -> str:
    if "```diff" in patch_text:
        m = re.search(r"```diff\n([\s\S]*?)\n```", patch_text)
        if m:
            return m.group(1).strip() + "\n"
    if "diff --git " in patch_text:
        idx = patch_text.find("diff --git ")
        return patch_text[idx:].strip() + "\n"
    return patch_text.strip() + "\n"


def changed_files_from_patch(patch: str) -> List[str]:
    files = []
    for line in patch.splitlines():
        if line.startswith("diff --git a/"):
            m = re.match(r"diff --git a/(.+?) b/(.+)", line)
            if m:
                files.append(m.group(2))
    return files


def _apply_search_replace_blocks(blocks: List[Dict[str, str]]) -> Tuple[bool, str]:
    """Apply search/replace blocks to files. Returns (success, message).
    Tries exact match first, then normalized whitespace, then indent-agnostic."""
    applied = 0
    errors: List[str] = []
    for block in blocks:
        fpath = ROOT / block["file"]
        if not fpath.exists():
            errors.append(f"File not found: {block['file']}")
            continue
        content = fpath.read_text(encoding="utf-8")
        search = block["search"]
        replace = block["replace"]

        # Try 1: exact match
        if search in content:
            content = content.replace(search, replace, 1)
            fpath.write_text(content, encoding="utf-8")
            applied += 1
            continue

        # Try 2: strip trailing whitespace per line
        search_norm = "\n".join(l.rstrip() for l in search.split("\n"))
        content_norm = "\n".join(l.rstrip() for l in content.split("\n"))
        if search_norm in content_norm:
            content = content_norm.replace(search_norm, replace, 1)
            fpath.write_text(content, encoding="utf-8")
            applied += 1
            continue

        # Try 3: indent-agnostic match (strip all leading whitespace, then find
        # the region in the original file that matches this stripped version)
        search_stripped = "\n".join(l.strip() for l in search.split("\n") if l.strip())
        content_lines = content.split("\n")
        search_lines = [l.strip() for l in search.split("\n") if l.strip()]
        match_start = -1
        for i in range(len(content_lines) - len(search_lines) + 1):
            candidate = [content_lines[i + j].strip() for j in range(len(search_lines))]
            if candidate == search_lines:
                match_start = i
                break
        if match_start >= 0:
            # Found the region — replace those lines
            match_end = match_start + len(search_lines)
            # Detect the indentation of the first matched line
            first_line = content_lines[match_start]
            indent = first_line[:len(first_line) - len(first_line.lstrip())]
            # Apply the replacement with the detected indentation
            replace_lines = replace.split("\n")
            # Re-indent replacement to match original
            reindented = []
            for rl in replace_lines:
                if rl.strip():
                    reindented.append(indent + rl.lstrip())
                else:
                    reindented.append("")
            content_lines[match_start:match_end] = reindented
            content = "\n".join(content_lines)
            fpath.write_text(content, encoding="utf-8")
            applied += 1
            continue

        errors.append(f"Search block not found in {block['file']} (first 80 chars: {search[:80]!r})")
    if errors:
        return applied > 0, f"Applied {applied} blocks, {len(errors)} errors: " + "; ".join(errors)
    return True, f"Applied {applied} search/replace blocks successfully."


def _parse_search_replace_response(raw: str) -> List[Dict[str, str]]:
    """Parse LLM response containing FILE/SEARCH/REPLACE blocks.
    Supports three formats:
      1. FILE: path\nSEARCH:\n<<<\n...\n>>>\nREPLACE:\n<<<\n...\n>>>
      2. Markdown: #### File: `path`\n**SEARCH:**\n```cpp\n...\n```\n**REPLACE:**\n```cpp\n...\n```
      3. Patch headers: ### PATCH: `path`\n#### SEARCH\n```cpp\n...\n```\n#### REPLACE\n```cpp\n...\n```
    Tolerates trailing whitespace on lines (common LLM artifact)."""
    blocks: List[Dict[str, str]] = []
    # Normalize: strip trailing whitespace from each line
    cleaned = "\n".join(line.rstrip() for line in raw.split("\n"))

    # Format 1: FILE: <path>\nSEARCH:\n<<<\n...\n>>>\nREPLACE:\n<<<\n...\n>>>
    pattern1 = re.compile(
        r"FILE:\s*(.+?)\s*\n"
        r"SEARCH:\s*\n<<<\n(.*?)\n>>>\s*\n"
        r"REPLACE:\s*\n<<<\n(.*?)\n>>>",
        re.DOTALL,
    )
    for m in pattern1.finditer(cleaned):
        blocks.append({
            "file": m.group(1).strip().strip("`"),
            "search": m.group(2),
            "replace": m.group(3),
        })

    if blocks:
        return blocks

    # Format 2: Markdown code fences (fallback)
    # Matches: **File:**/#### File: `path`  +  **SEARCH:**```..```  +  **REPLACE:**```..```
    pattern2 = re.compile(
        r"(?:#{1,4}\s*)?(?:\*{0,2})File(?:\*{0,2})[:\s]*[`\"]?([^`\"\n]+?)[`\"]?\s*\n"
        r".*?\*{0,2}SEARCH\*{0,2}[:\s]*\n```[a-z]*\n(.*?)\n```\s*\n"
        r".*?\*{0,2}REPLACE\*{0,2}[:\s]*\n```[a-z]*\n(.*?)\n```",
        re.DOTALL,
    )
    for m in pattern2.finditer(cleaned):
        blocks.append({
            "file": m.group(1).strip().strip("`"),
            "search": m.group(2),
            "replace": m.group(3),
        })

    if blocks:
        return blocks

    # Format 3: PATCH headers with #### SEARCH / #### REPLACE code fences
    # Matches: ### PATCH: `path`\n\n#### SEARCH\n```cpp\n...\n```\n\n#### REPLACE\n```cpp\n...\n```
    pattern3 = re.compile(
        r"#{1,4}\s*PATCH[:\s]*[`\"]?([^`\"\n]+?)[`\"]?\s*\n"
        r"[\s\S]*?#{1,4}\s*SEARCH\s*\n```[a-z]*\n(.*?)\n```\s*\n"
        r"[\s\S]*?#{1,4}\s*REPLACE\s*\n```[a-z]*\n(.*?)\n```",
        re.DOTALL,
    )
    for m in pattern3.finditer(cleaned):
        blocks.append({
            "file": m.group(1).strip().strip("`"),
            "search": m.group(2),
            "replace": m.group(3),
        })

    return blocks


def _extract_api_inventory(file_contents: str) -> str:
    """Extract function/method/type names from file content to build a grounding checklist."""
    names: set = set()
    for m in re.finditer(r'\b([A-Z]\w*::\w+)\s*\(', file_contents):
        names.add(m.group(1))
    for m in re.finditer(r'(\w+)\s*\.\s*(\w+)\s*\(', file_contents):
        names.add(f"{m.group(1)}.{m.group(2)}()")
    for m in re.finditer(r'(\w+)\s*->\s*(\w+)\s*\(', file_contents):
        names.add(f"{m.group(1)}->{m.group(2)}()")
    for m in re.finditer(r'\bvoid\s+(\w+(?:::\w+)*)\s*\(', file_contents):
        names.add(m.group(1))
    for m in re.finditer(r'\b(vk\w+)\s*\(', file_contents):
        names.add(m.group(1))
    return ", ".join(sorted(names)[:60]) if names else "(no APIs extracted)"


def generate_patch(task_md: str, outputs: Dict[str, str], allowed_files: List[str]) -> str:
    """Generate a patch using search/replace blocks applied to real files, then git diff."""
    aggregated = "\n\n".join([f"## {k}\n{v}" for k, v in outputs.items()])
    allowed = "\n".join([f"- `{p}`" for p in allowed_files])

    # Provide actual file content for files referenced in code proposals
    referenced_files: List[str] = []
    for path_str in allowed_files:
        p = ROOT / path_str
        if p.exists():
            for _, text in outputs.items():
                if path_str in text or p.name in text:
                    referenced_files.append(path_str)
                    break
    file_contents = ""
    char_budget = 12000
    per_file_cap = 3000
    for ref in referenced_files[:6]:
        p = ROOT / ref
        if p.exists():
            content = read_capped(p, max_chars=per_file_cap)
            file_contents += f"\n### {ref}\n```\n{content}\n```\n"
            char_budget -= len(content)
            if char_budget <= 0:
                break

    # Build API inventory from actual file contents for grounding
    api_inventory = _extract_api_inventory(file_contents)

    prompt = f"""
You are a patch generator. You read ACTUAL SOURCE FILES and produce SEARCH/REPLACE edits.

# AUTHORITATIVE SOURCE — The Real Files
These are the ACTUAL files from the repository. Your SEARCH blocks MUST be copied
CHARACTER FOR CHARACTER from these files. Your REPLACE blocks may ONLY call
functions, methods, and types that appear in these files.

{file_contents}

# Available APIs (extracted from files above)
{api_inventory}

# Task Intent
{task_md}

# Agent Proposals (INTENT ONLY — do NOT copy their code literally)
The agents below describe WHAT should change, but their code examples may reference
functions that DO NOT EXIST. Treat proposals as English-language intent descriptions.
Extract the GOAL (e.g. "add error handling", "add logging") but write your REPLACE
blocks using ONLY the APIs visible in the Authoritative Source above.
If an agent proposes calling `foo->bar()` but `bar()` does not appear in any file
above, DO NOT include that call. Use an existing API that achieves the same goal,
or skip that part of the proposal.

{aggregated}

# Allowed Files
{allowed}

# Output Format
Return one or more blocks in this EXACT format (no markdown, no prose, no headers):

FILE: path/to/file.cpp
SEARCH:
<<<
exact lines copied from Authoritative Source above
>>>
REPLACE:
<<<
new lines using ONLY APIs from Available APIs list above
>>>

WRONG format (DO NOT use this):
  #### File: `path`
  **SEARCH:**
  ```cpp
  ...
  ```
  **REPLACE:**
  ```cpp
  ...
  ```

CORRECT format uses FILE:/SEARCH:/REPLACE: with <<< >>> delimiters. No markdown formatting.

Rules:
- SEARCH text must be COPIED VERBATIM from the files shown above — same whitespace, same indentation, same line breaks
- REPLACE blocks may ONLY call functions/methods/types listed in Available APIs or standard C++ (try/catch, if, etc.)
- Do NOT reference member variables that don't appear in the file contents above
- If an agent proposal calls a method not in Available APIs, SKIP that call or substitute an existing equivalent
- Include 2-3 lines of unchanged context before and after the changed lines in SEARCH
- Keep changes minimal — smallest viable diff
- No prose, no explanation — only FILE/SEARCH/REPLACE blocks
""".strip()
    raw = call_llm(prompt)

    # Parse and apply search/replace blocks
    blocks = _parse_search_replace_response(raw)
    if not blocks:
        # Fallback: try extracting a unified diff the old way
        return extract_patch(raw)

    # Validate files are in allowed scope
    allowed_set = set(allowed_files)
    for block in blocks:
        if block["file"] not in allowed_set:
            return f"# Patch rejected: block references disallowed file {block['file']}\n"

    # Log API grounding warnings for REPLACE blocks
    api_names = set(re.findall(r'\b(\w+(?:::\w+)*)\s*\(', file_contents))
    for block in blocks:
        replace_calls = set(re.findall(r'(\w+(?:::\w+|->\w+|\.\w+)*)\s*\(', block["replace"]))
        # Filter to non-trivial calls (skip keywords, standard constructs)
        skip = {"if", "for", "while", "switch", "catch", "try", "return", "throw", "sizeof", "static_cast", "dynamic_cast", "reinterpret_cast", "const_cast"}
        novel = {c for c in replace_calls if c.split("::")[-1].split("->")[-1].split(".")[-1] not in skip
                 and not any(known in c for known in api_names)
                 and c not in api_names}
        if novel:
            print(f"  ⚠ Patch grounding: REPLACE block for {block['file']} introduces calls not in source files: {', '.join(sorted(novel))}")

    ok, msg = _apply_search_replace_blocks(blocks)
    if not ok:
        return f"# Search/replace failed: {msg}\n"

    # Generate real diff from working tree
    code, diff_out = run_command("git diff")
    if code != 0 or not diff_out.strip():
        # Revert and report
        run_command("git checkout -- .")
        return f"# No diff generated after applying blocks. Message: {msg}\n"

    # Revert working tree — the guarded apply will re-apply from the patch file
    run_command("git checkout -- .")
    return diff_out.strip() + "\n"


def gather_numstat() -> Tuple[int, int, int]:
    code, out = run_command("git diff --numstat")
    if code != 0:
        return 0, 0, 0
    files = 0
    adds = 0
    dels = 0
    for line in out.splitlines():
        parts = line.split("\t")
        if len(parts) < 3:
            continue
        a, d, _ = parts
        try:
            adds += int(a)
            dels += int(d)
        except ValueError:
            continue
        files += 1
    return files, adds, dels


def apply_patch_guarded(patch_path: Path, allowed_files: List[str], run_log_path: Path) -> Tuple[bool, str]:
    patch_raw = read_text(patch_path)
    touched = changed_files_from_patch(patch_raw)
    if not touched:
        return False, "Patch contains no file changes."

    touched_set = sorted(set(touched))
    allowed_set = set(allowed_files)
    disallowed = [f for f in touched_set if f not in allowed_set]
    if disallowed:
        return False, f"Patch touched disallowed files: {', '.join(disallowed)}"

    if len(touched_set) > MAX_PATCH_FILES:
        return False, f"Patch touches {len(touched_set)} files, above limit {MAX_PATCH_FILES}."

    code, out = run_command(f"git apply --whitespace=fix '{patch_path.as_posix()}'")
    if code != 0:
        return False, f"git apply failed:\n{out}"

    files, adds, dels = gather_numstat()
    if files > MAX_PATCH_FILES:
        run_command("git reset --hard HEAD")
        return False, f"Changed files {files} above limit {MAX_PATCH_FILES}."

    if (adds + dels) > MAX_PATCH_LINES:
        run_command("git reset --hard HEAD")
        return False, f"Changed lines {adds + dels} above limit {MAX_PATCH_LINES}."

    if GUARD_BUILD_CMD:
        code, out = run_command(GUARD_BUILD_CMD)
        if code != 0:
            run_command("git reset --hard HEAD")
            run_log_path.write_text(out, encoding="utf-8")
            return False, "Build guard failed. See guard log."

    return True, f"Patch applied. files={files}, additions={adds}, deletions={dels}"


def normalize_agent_name(raw: str) -> str:
    key = raw.strip().lower()
    aliases = {
        "lead++": "C++ Lead",
        "cpp": "C++ Lead",
        "cpplead": "C++ Lead",
        "c++": "C++ Lead",
        "c++ lead": "C++ Lead",
        "vulkan": "Vulkan Guru",
        "vulkan guru": "Vulkan Guru",
        "kernel": "Kernel Expert",
        "kernel expert": "Kernel Expert",
        "refactor": "Refactorer",
        "refactorer": "Refactorer",
        "guild": "Guild Master",
        "guild master": "Guild Master",
        "guildmaster": "Guild Master",
        "master": "Guild Master",
    }
    return aliases.get(key, raw.strip())


_AGENT_SCOPE_RE = re.compile(
    r"\b(agents?|prompt.?catalog|macros?[._]|workflow|schedule|guild|party|town|orchestrat)",
    re.IGNORECASE,
)
_DOC_SCOPE_RE = re.compile(
    r"\b(readme|documentation|docs|changelog|license|contributing)",
    re.IGNORECASE,
)


def _detect_scope_block(task_command: str) -> str:
    """Choose scope files based on what the task_command is about."""
    if _AGENT_SCOPE_RE.search(task_command):
        return (
            "- In scope:\n"
            "    - `.github/agents/party/*`\n"
            "    - `.github/agents/town/*`\n"
            "    - `.github/agents/guilds/*`\n"
            "    - `.github/scripts/agent_autorun.py`\n"
            "    - `.github/workflows/agent-autonomous-run.yml`\n"
            "- Out of scope:\n"
            "    - C++ source code\n"
            "    - Shader files\n"
            "    - Third-party libraries"
        )
    if _DOC_SCOPE_RE.search(task_command):
        return (
            "- In scope:\n"
            "    - `README.md`\n"
            "    - `.github/agents/town/*`\n"
            "    - `assets/economic/*`\n"
            "- Out of scope:\n"
            "    - C++ source code\n"
            "    - Shader files\n"
            "    - Third-party libraries"
        )
    # Default: C++ engine scope
    return (
        "- In scope:\n"
        "    - `src/engine/*`\n"
        "    - `src/vulkan_base/*`\n"
        "    - `src/vulkan_mechanics/*`\n"
        "    - `src/vulkan_pipelines/*`\n"
        "    - `src/vulkan_resources/*`\n"
        "    - `src/world/*`\n"
        "    - `shaders/Engine.comp`\n"
        "    - `shaders/PostFX.comp`\n"
        "    - `shaders/ParameterUBO.glsl`\n"
        "- Out of scope:\n"
        "    - Third-party libraries\n"
        "    - Unrelated feature work"
    )


def apply_task_command_override(task_md: str, task_command: str, macro_mode: str) -> str:
        if not task_command:
                return task_md

        task_id = f"{(macro_mode or 'TASK').upper()}-{re.sub(r'[^A-Za-z0-9]+', '-', task_command).strip('-')[:24].upper() or 'CMD'}"
        scope_block = _detect_scope_block(task_command)

        overridden = f"""
# Current Manual Task

## Task ID
{task_id}

## Manual Action Statement
{task_command}

## Scope
{scope_block}

## Sequential Execution Order
1. C++ Lead
2. Vulkan Guru
3. Kernel Expert
4. Refactorer

## C++ Lead Required Handoff TODOs
After C++ Lead completes main + secondary tasks, create TODO blocks for:
- Vulkan Guru TODOs
- Kernel Expert TODOs
- Refactorer TODOs

## Output Location
- Run report: `.github/agents/runs/YYYY-MM-DD-choice-a.md`
- PR links: add under each agent section in the run report
""".strip()
        return overridden


def main() -> None:
    today = dt.date.today().isoformat()

    # Route macro mode to agent selection (driven by macros.json schema)
    agent_only = AGENT_ONLY
    agent_set = AGENT_SET

    if MACRO_MODE:
        macro = MACRO_MODE.lower().strip()
        macro_agents = get_macro_agents(macro)
        if macro_agents is not None:
            agent_only = ""
            agent_set = ",".join(macro_agents)
        else:
            print(f"  Warning: unknown macro '{macro}', falling back to env config")

    base_md = read_text(TOWN_DIR / "base.md")
    readme_md = read_text(TOWN_DIR / "README.md")
    schedule_md = read_text(TOWN_DIR / "schedule.md")
    task_md_main = read_text(TOWN_DIR / "current-task.md")

    if TASK_MODE == "self":
        task_md = build_self_task(schedule_md, readme_md)
    else:
        task_md = task_md_main

    task_md = apply_task_command_override(task_md, TASK_COMMAND, MACRO_MODE)

    task_id = extract_task_id(task_md)
    scope_files = parse_scope_files(task_md)
    scope_rel = [str(p.relative_to(ROOT)) for p in scope_files]
    context_chars = get_macro_knob(MACRO_MODE, "context_chars", MAX_CODE_CONTEXT)
    file_cap = get_macro_knob(MACRO_MODE, "per_file_cap", 3000)

    # Extract task keywords for relevance-based context ordering
    task_keywords = [w.lower() for w in re.findall(r'[A-Za-z]{3,}', TASK_COMMAND or "")]
    scoped_code = build_scoped_code(scope_files, max_chars=context_chars,
                                     task_keywords=task_keywords,
                                     per_file_cap=file_cap)

    all_agents: List[Tuple[str, str]] = [
        ("C++ Lead", "party/cpp-lead.md"),
        ("Vulkan Guru", "party/vulkan-guru.md"),
        ("Kernel Expert", "party/kernel-expert.md"),
        ("Refactorer", "party/refactorer.md"),
        ("Guild Master", "guilds/guild-master.md"),
    ]

    sequence: List[Tuple[str, str]] = [
        ("C++ Lead", "party/cpp-lead.md"),
        ("Vulkan Guru", "party/vulkan-guru.md"),
        ("Kernel Expert", "party/kernel-expert.md"),
        ("Refactorer", "party/refactorer.md"),
    ]

    selected_agent = normalize_agent_name(agent_only) if agent_only else ""
    selected_set = [normalize_agent_name(x) for x in agent_set.split(",") if x.strip()]

    if selected_set:
        requested = set(selected_set)
        sequence = [entry for entry in all_agents if entry[0] in requested]
        missing = [name for name in requested if name not in {n for n, _ in all_agents}]
        if missing:
            valid = ", ".join([name for name, _ in all_agents])
            raise SystemExit(f"Unknown AGENT_SET entries: {', '.join(missing)}. Valid values: {valid}")
    elif selected_agent:
        sequence = [entry for entry in all_agents if entry[0] == selected_agent]
        if not sequence:
            valid = ", ".join([name for name, _ in all_agents])
            raise SystemExit(f"Unknown AGENT_ONLY '{agent_only}'. Valid values: {valid}")

    if selected_agent == "Guild Master":
        scoped_code = build_guild_master_context()

    is_think = is_independent_macro(MACRO_MODE)
    temperature = get_temperature(MACRO_MODE)
    macro_handoff_chars = get_macro_knob(MACRO_MODE, "handoff_chars", 1500)
    macro_cross_confirm = get_macro_knob(MACRO_MODE, "cross_confirm", True)
    macro_retry = get_macro_knob(MACRO_MODE, "retry_on_gate_fail", True)
    macro_max_chars = get_macro_knob(MACRO_MODE, "max_output_chars", 4000)
    macro_sections = get_macro_knob(MACRO_MODE, "required_sections", None)
    run_id = f"{today}-{(MACRO_MODE or 'run').lower()}-a"
    outputs: Dict[str, str] = {}
    full_next_runs: Dict[str, str] = {}  # captured before truncation
    handoff_todos = "Start from task statement; no upstream TODOs yet."
    pipeline_halted = False
    halt_message = ""

    for name, filename in sequence:
        if pipeline_halted:
            outputs[name] = f"*Skipped — pipeline halted: {halt_message}*"
            continue

        profile_md = read_text(AGENTS_DIR / filename)
        guild_context = load_guild_context(name)
        macro_directive = get_macro_directive(MACRO_MODE, name)

        # Think macro: each agent works independently (no previous outputs)
        if is_think:
            agent_outputs: Dict[str, str] = {}  # empty — isolation
            agent_todos = "You are working INDEPENDENTLY. No prior agent output is available. Propose your own approach."
        else:
            agent_outputs = outputs
            agent_todos = handoff_todos

        # Determinism: check cache before calling LLM
        fingerprint = compute_fingerprint(task_md, scoped_code, name, MACRO_MODE)
        cached = cache_lookup(fingerprint)
        t_start = time.time()

        if cached is not None:
            result = cached
            cache_hit = True
            print(f"  ⚡ Cache hit for {name} (fingerprint {fingerprint})")
        else:
            prompt = build_agent_prompt(
                base_md,
                profile_md,
                task_md,
                scoped_code,
                agent_outputs,
                agent_todos,
                name,
                guild_context,
                macro_directive,
                handoff_chars=macro_handoff_chars,
                cross_confirm=macro_cross_confirm,
                macro_sections=macro_sections,
            )

            result = call_llm(prompt, temperature=temperature)
            cache_hit = False

            # --- Quality Gate: retry once if structure is wrong ---
            gate_action, gate_msg, gate_extras = run_quality_gate(name, result, MACRO_MODE)

            if gate_action == "RETRY" and macro_retry:
                print(f"  🔄 Gate RETRY for {name}: {gate_msg}")
                retry_prompt = prompt + f"\n\n# RETRY INSTRUCTION\nYour previous output was rejected: {gate_msg}\nRewrite your complete output with ALL required sections."
                result = call_llm(retry_prompt, temperature=temperature)
                gate_action, gate_msg, gate_extras = run_quality_gate(name, result, MACRO_MODE)
                if gate_action == "RETRY":
                    print(f"  ⚠ Second retry still failing for {name}, proceeding anyway")
                    gate_action = "WARN"
            elif gate_action == "RETRY" and not macro_retry:
                print(f"  ℹ Gate RETRY skipped for {name} (retry_on_gate_fail=false): {gate_msg}")
                gate_action = "WARN"

            if gate_action == "HALT":
                print(f"  🛑 Gate HALT for {name}: {gate_msg}")
                pipeline_halted = True
                halt_message = f"{name}: {gate_msg}"
            elif gate_action == "WARN":
                print(f"  ⚠ Gate WARN for {name}: {gate_msg}")
            else:
                print(f"  ✅ Gate PASS for {name}")

            # Extract next-run from FULL output BEFORE truncation
            next_run_match = re.search(
                r"(?:## Next Run|9\)\s*Recommended Next Run)([\s\S]*?)(?:\n## |\n\d+\)|\Z)",
                result,
                flags=re.IGNORECASE,
            )
            if next_run_match:
                _full_next_run = next_run_match.group(1).strip()
                full_next_runs[name] = _full_next_run

            # Truncate excessive output (per-macro cap)
            if len(result) > macro_max_chars:
                result = result[:macro_max_chars] + "\n\n... [output truncated by factory gate] ..."
                print(f"  ✂ Output truncated for {name} ({len(result)} → {macro_max_chars} chars)")

            # Cache the (possibly retried) result
            cache_store(fingerprint, result)

        latency_ms = int((time.time() - t_start) * 1000)

        # --- Metrics ---
        concur, qualify, dissent = _count_dissent_markers(result)
        confidence = _extract_confidence(result)
        metric_prompt_chars = 0
        metric_gate = "CACHE"
        metric_sections = _count_sections(result)
        if not cache_hit:
            metric_prompt_chars = len(prompt)
            metric_gate = gate_action
            metric_sections = gate_extras.get("sections_found", metric_sections)
        append_metric({
            "timestamp": dt.datetime.utcnow().isoformat() + "Z",
            "run_id": run_id,
            "agent": name,
            "macro": MACRO_MODE or "none",
            "task_id": task_id,
            "fingerprint": fingerprint,
            "latency_ms": latency_ms,
            "prompt_chars": metric_prompt_chars,
            "output_chars": len(result),
            "gate_result": metric_gate,
            "sections_found": metric_sections,
            "concur_count": concur,
            "qualify_count": qualify,
            "dissent_count": dissent,
            "confidence": confidence,
            "cache_hit": cache_hit,
        })

        outputs[name] = result

        if name == "C++ Lead":
            handoff_todos = "\n\n".join(
                [
                    f"Vulkan Guru TODOs:\n{extract_todos_for('Vulkan Guru', result)}",
                    f"Kernel Expert TODOs:\n{extract_todos_for('Kernel Expert', result)}",
                    f"Refactorer TODOs:\n{extract_todos_for('Refactorer', result)}",
                ]
            )
        else:
            handoff_todos = f"Carry forward previous findings and TODOs. Latest output from {name}:\n{result[:1800]}"

    RUNS_DIR.mkdir(parents=True, exist_ok=True)
    PROPOSALS_DIR.mkdir(parents=True, exist_ok=True)
    LOGS_DIR = RUNS_DIR / "logs"
    LOGS_DIR.mkdir(parents=True, exist_ok=True)

    # ---------------------------------------------------------------------------
    # Split raw outputs into public (PR-facing) and reasoning (log-only)
    # ---------------------------------------------------------------------------
    public_outputs: Dict[str, str] = {}
    reasoning_outputs: Dict[str, str] = {}
    for name, _ in sequence:
        raw = outputs.get(name, "")
        pub, reasoning = strip_reasoning(raw)
        public_outputs[name] = pub
        reasoning_outputs[name] = reasoning

    # Save full raw outputs + reasoning to log file (NOT committed to PR)
    detail_log_path = LOGS_DIR / f"{today}-{task_id.lower()}-detail.md"
    detail_lines = [f"# Full Agent Output Log — {task_id} ({today})", ""]
    for name, _ in sequence:
        detail_lines.append(f"## {name} — Full Output")
        detail_lines.append(outputs.get(name, ""))
        detail_lines.append("")
        if reasoning_outputs.get(name):
            detail_lines.append(f"## {name} — Reasoning")
            detail_lines.append(reasoning_outputs[name])
            detail_lines.append("")
    detail_log_path.write_text("\n".join(detail_lines).strip() + "\n", encoding="utf-8")

    # Extract and append new procedures discovered by agents
    procedures_path = TOWN_DIR / "procedures.md"
    new_procedures = []
    for name, _ in sequence:
        output = outputs.get(name, "")
        m = re.search(r"(?:Procedure Recording|## Procedures?)([\s\S]*?)(?:\n## |\Z)", output, flags=re.IGNORECASE)
        if m:
            proc_text = m.group(1).strip()
            if proc_text and "no new" not in proc_text.lower() and "none" not in proc_text.lower() and len(proc_text) > 20:
                new_procedures.append(f"\n### From {name} ({today}, {task_id})\n{proc_text}\n")
    if new_procedures:
        with open(procedures_path, "a", encoding="utf-8") as f:
            for proc in new_procedures:
                f.write(proc)

    # --- Guild Master governance: extract and apply policy decisions ---
    gm_output = outputs.get("Guild Master", "")
    if gm_output and not gm_output.startswith("*Skipped"):
        gm_actions = extract_guild_master_actions(gm_output)
        gov_summary = apply_guild_master_actions(gm_actions, run_id)
        print(f"  🏛️  {gov_summary}")

    # --- Guild Review: lightweight post-sequence quality check ---
    guild_review_result: Dict[str, str] = {}
    macro_guild_review = get_macro_knob(MACRO_MODE, "guild_review", False)
    if macro_guild_review and not pipeline_halted:
        print("  🔍 Running Guild Review (post-sequence quality check)...")
        guild_review_result = run_guild_review(task_md, outputs, scoped_code, MACRO_MODE)
        verdict = guild_review_result.get("verdict", "APPROVE")
        summary = guild_review_result.get("summary", "")
        halluc = guild_review_result.get("hallucination_flags", "")
        print(f"  🏛️  Guild Review: {verdict} — {summary}")
        if halluc:
            print(f"  ⚠ Hallucination flags: {halluc}")
        if verdict == "BLOCK":
            pipeline_halted = True
            halt_message = f"Guild Review BLOCK: {summary}"
            print(f"  🛑 Guild Review blocked the pipeline: {summary}")

        # Log the review
        review_log_path = LOGS_DIR / f"{today}-{task_id.lower()}-guild-review.md"
        review_log_path.write_text(
            f"# Guild Review — {task_id} ({today})\n\n"
            f"**Verdict**: {verdict}\n"
            f"**Summary**: {summary}\n"
            f"**Hallucination Flags**: {halluc or 'none'}\n\n"
            f"## Full Review\n{guild_review_result.get('raw', '')}\n",
            encoding="utf-8",
        )

    # --- Next-task preparation: agents recommend what to run next ---
    next_run_recs = extract_next_run(outputs, full_next_runs)
    next_task_path = TOWN_DIR / "next-task.md"
    if next_run_recs and not pipeline_halted:
        next_task_content = (
            f"# Recommended Next Run\n\n"
            f"Generated by agent run `{run_id}` on {today}.\n\n"
            f"{next_run_recs}\n\n"
            f"Run via GitHub Actions **Run workflow** or:\n\n"
            f"```fish\n"
            f"gh workflow run \"Agent Autonomous Run\" --ref dev-linux \\\n"
            f"  -f macro=<Macro> -f task_command=\"<command>\"\n"
            f"```\n"
        )
        next_task_path.write_text(next_task_content, encoding="utf-8")
        print(f"  📋 Next-task recommendations written to {next_task_path.relative_to(ROOT)}")
    elif next_task_path.exists():
        next_task_path.write_text(
            f"# Recommended Next Run\n\nNo follow-up recommended from run `{run_id}` ({today}).\n",
            encoding="utf-8",
        )

    # ---------------------------------------------------------------------------
    # Build slim human-readable run report (PR-facing)
    # ---------------------------------------------------------------------------
    report_path = RUNS_DIR / f"{today}-choice-a.md"
    proposal_md_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-proposal.md"
    proposal_patch_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-proposal.patch"
    guard_log_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-guard.log"

    status_str = "HALTED — " + halt_message if pipeline_halted else "completed"
    report_lines = [
        f"# {task_id} — {today}",
        "",
        f"**Macro**: {MACRO_MODE or 'none'} | "
        f"**Agents**: {' → '.join(name for name, _ in sequence)} | "
        f"**Status**: {status_str}",
        "",
    ]

    # Per-agent one-liner summary
    report_lines.append("## Agent Summary")
    report_lines.append("")
    report_lines.append("| Agent | Finding |")
    report_lines.append("|-------|---------|")
    for name, _ in sequence:
        one_liner = extract_one_liner(public_outputs.get(name, ""))
        report_lines.append(f"| {name} | {one_liner} |")
    report_lines.append("")

    # Combined TODOs (deduplicated across agents)
    combined_todos = aggregate_todos(public_outputs)
    report_lines.append("## TODOs")
    report_lines.append("")
    report_lines.append(combined_todos)
    report_lines.append("")

    # Per-agent public output (reasoning stripped)
    for name, _ in sequence:
        pub = public_outputs.get(name, "").strip()
        if pub:
            report_lines.append(f"## {name}")
            report_lines.append(pub)
            report_lines.append("")

    # Next-run recommendations in the report
    if next_run_recs:
        report_lines.append("## Next Run")
        report_lines.append("")
        report_lines.append(next_run_recs)
        report_lines.append("")

    # Guild Review summary in report
    if guild_review_result:
        report_lines.append("## Guild Review")
        report_lines.append("")
        report_lines.append(f"**Verdict**: {guild_review_result.get('verdict', 'N/A')}")
        report_lines.append(f"**Summary**: {guild_review_result.get('summary', 'N/A')}")
        halluc = guild_review_result.get("hallucination_flags", "")
        if halluc:
            report_lines.append(f"**Hallucination Flags**: {halluc}")
        report_lines.append("")

    # Factory metrics dashboard
    trailing = load_trailing_metrics(days=7)
    report_lines.append(compute_metrics_summary(trailing))
    report_lines.append("")

    # Code proposal — only the code blocks, no commentary
    proposal_lines = [
        f"# Code Proposal — {task_id} ({today})",
        "",
    ]
    for name, _ in sequence:
        pub = public_outputs.get(name, "")
        # Extract code blocks from ## Code Proposal or ## TODOs sections
        m = re.search(r"(?:Code Proposal|## Code)([\s\S]*?)(?:\n## |\Z)", pub, flags=re.IGNORECASE)
        code_section = m.group(1).strip() if m else ""
        if code_section:
            proposal_lines.append(f"## {name}")
            proposal_lines.append(code_section)
            proposal_lines.append("")

    report_path.write_text("\n".join(report_lines).strip() + "\n", encoding="utf-8")
    proposal_md_path.write_text("\n".join(proposal_lines).strip() + "\n", encoding="utf-8")

    macro_produces_patch = MACRO_DEFS.get(MACRO_MODE, {}).get("produces_patch", True)
    if macro_produces_patch:
        patch_text = generate_patch(task_md, outputs, scope_rel)
        proposal_patch_path.write_text(patch_text, encoding="utf-8")
    else:
        patch_text = ""
        proposal_patch_path.write_text("# No patch — this macro does not produce patches.\n", encoding="utf-8")

    apply_status = "not-requested"
    apply_message = ""
    if AUTO_APPLY_PATCH:
        ok, msg = apply_patch_guarded(proposal_patch_path, scope_rel, guard_log_path)
        apply_status = "applied" if ok else "guard-rejected"
        apply_message = msg

    summary = {
        "task": task_id,
        "macro_mode": MACRO_MODE if MACRO_MODE else "none",
        "task_command": TASK_COMMAND if TASK_COMMAND else "",
        "task_mode": TASK_MODE,
        "agent_mode": selected_agent if selected_agent else ("set:" + ",".join(selected_set) if selected_set else "all"),
        "run_report": str(report_path.relative_to(ROOT)),
        "detail_log": str(detail_log_path.relative_to(ROOT)),
        "code_proposal": str(proposal_md_path.relative_to(ROOT)),
        "patch_proposal": str(proposal_patch_path.relative_to(ROOT)),
        "auto_apply_patch": AUTO_APPLY_PATCH,
        "apply_status": apply_status,
        "apply_message": apply_message,
        "pipeline_halted": pipeline_halted,
        "halt_message": halt_message if pipeline_halted else "",
        "guild_review_verdict": guild_review_result.get("verdict", "") if guild_review_result else "",
        "guild_review_summary": guild_review_result.get("summary", "") if guild_review_result else "",
        "allowed_scope_files": scope_rel,
        "agents": [name for name, _ in sequence],
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
