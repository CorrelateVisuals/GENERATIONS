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
    found = 0
    for section in REQUIRED_SECTIONS:
        if ")" in section:
            text_part = section.split(")", 1)[-1].strip()
            pattern = re.escape(section) + r"|" + re.escape(text_part)
        else:
            pattern = re.escape(section)
        if re.search(pattern, output, re.IGNORECASE):
            found += 1
    return found


def _count_sections_list(output: str, sections: List[str]) -> int:
    """Count how many sections from a specific list are present."""
    found = 0
    for section in sections:
        if ")" in section:
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
    for m in re.finditer(r"(?:^|[\s`])([a-zA-Z]\S+\.(?:cpp|h|hpp|comp|frag|vert|glsl|tesc|tese))(?:[\s`]|$)", output, re.MULTILINE):
        path = ROOT / m.group(1)
        if not path.exists() and not (ROOT / "src" / m.group(1)).exists():
            bad.append(m.group(1))
    return bad


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
    "HPC Marketeer": [],
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


def _llm_request(prompt: str, temperature: float = 0.2) -> str:
    """Single LLM API call (no retry). Both providers use the same chat/completions format."""
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


def extract_next_run(outputs: Dict[str, str]) -> str:
    """Extract and consolidate next-run recommendations from all agent outputs.
    Returns a ready-to-use next-task.md or empty string if no recommendations."""
    recs: List[str] = []
    for name, text in outputs.items():
        m = re.search(
            r"9\)\s*Recommended Next Run([\s\S]*?)(?:\n## |\n\d+\)|\Z)",
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


def build_scoped_code(files: List[Path], max_chars: int = 12000) -> str:
    if not files:
        return "No in-scope files found."
    chunks = []
    total = 0
    for f in files:
        rel = f.relative_to(ROOT)
        snippet = read_capped(f)
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
    sections_default = """Return markdown with these exact sections:
1) Main Task Outcome
2) Secondary Task Outcomes
3) Risks and Constraints
4) Actionable TODOs (use Structured Handoff Format from base rules)
5) Handoff Note
6) Code Proposal
7) Cross-Confirmation (if macro directive requires it)
8) Procedure Recording (only if you discovered a new reusable pattern)
9) Recommended Next Run (suggest the next macro + task_command for the human)
   Format: `<Macro> "<task_command>"` with a one-line rationale.
   Example: `Follow "implement descriptor pool wrapper"` — blast radius mapped, ready to execute.
   If no follow-up needed, write: `None — task is self-contained.`"""

    if macro_sections:
        section_block = "Return markdown with these exact sections:\n"
        for s in macro_sections:
            section_block += f"{s}\n"
        if cross_confirm:
            section_block += "7) Cross-Confirmation (CONCUR/QUALIFY/DISSENT for prior agent findings)\n"
    else:
        section_block = sections_default
        if not cross_confirm:
            section_block = section_block.replace(
                "7) Cross-Confirmation (if macro directive requires it)\n", "")

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
5. HPC Marketeer

## C++ Lead Required Handoff TODOs
- Vulkan Guru TODOs
- Kernel Expert TODOs
- Refactorer TODOs
- HPC Marketeer TODOs

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


def generate_patch(task_md: str, outputs: Dict[str, str], allowed_files: List[str]) -> str:
    aggregated = "\n\n".join([f"## {k}\n{v}" for k, v in outputs.items()])
    allowed = "\n".join([f"- {p}" for p in allowed_files])
    prompt = f"""
Generate a minimal unified git patch for this task.
Only modify files from the allowed list.

# Active Task
{task_md}

# Agent Outputs
{aggregated}

# Allowed Files
{allowed}

Return only a valid git patch in unified diff format starting with `diff --git`.
No prose.
""".strip()
    raw = call_llm(prompt)
    return extract_patch(raw)


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
        "hpc": "HPC Marketeer",
        "hpc marketeer": "HPC Marketeer",
        "guild": "Guild Master",
        "guild master": "Guild Master",
        "guildmaster": "Guild Master",
        "master": "Guild Master",
    }
    return aliases.get(key, raw.strip())


def apply_task_command_override(task_md: str, task_command: str, macro_mode: str) -> str:
        if not task_command:
                return task_md

        task_id = f"{(macro_mode or 'TASK').upper()}-{re.sub(r'[^A-Za-z0-9]+', '-', task_command).strip('-')[:24].upper() or 'CMD'}"
        scope_block = """
- In scope:
    - `src/engine/*`
    - `src/vulkan_base/*`
    - `src/vulkan_mechanics/*`
    - `src/vulkan_pipelines/*`
    - `src/vulkan_resources/*`
    - `src/world/*`
    - `shaders/Engine.comp`
    - `shaders/PostFX.comp`
    - `shaders/ParameterUBO.glsl`
- Out of scope:
    - Third-party libraries
    - Unrelated feature work
""".strip()

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
5. HPC Marketeer

## C++ Lead Required Handoff TODOs
After C++ Lead completes main + secondary tasks, create TODO blocks for:
- Vulkan Guru TODOs
- Kernel Expert TODOs
- Refactorer TODOs
- HPC Marketeer TODOs

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
    scoped_code = build_scoped_code(scope_files, max_chars=context_chars)

    all_agents: List[Tuple[str, str]] = [
        ("C++ Lead", "party/cpp-lead.md"),
        ("Vulkan Guru", "party/vulkan-guru.md"),
        ("Kernel Expert", "party/kernel-expert.md"),
        ("Refactorer", "party/refactorer.md"),
        ("HPC Marketeer", "party/hpc-marketeer.md"),
        ("Guild Master", "guilds/guild-master.md"),
    ]

    sequence: List[Tuple[str, str]] = [
        ("C++ Lead", "party/cpp-lead.md"),
        ("Vulkan Guru", "party/vulkan-guru.md"),
        ("Kernel Expert", "party/kernel-expert.md"),
        ("Refactorer", "party/refactorer.md"),
        ("HPC Marketeer", "party/hpc-marketeer.md"),
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
            )

            result = call_llm(prompt, temperature=temperature)
            cache_hit = False

            # --- Quality Gate: retry once if structure is wrong ---
            gate_action, gate_msg, gate_extras = run_quality_gate(name, result, MACRO_MODE)

            if gate_action == "RETRY":
                print(f"  🔄 Gate RETRY for {name}: {gate_msg}")
                retry_prompt = prompt + f"\n\n# RETRY INSTRUCTION\nYour previous output was rejected: {gate_msg}\nRewrite your complete output with ALL required sections."
                result = call_llm(retry_prompt, temperature=temperature)
                gate_action, gate_msg, gate_extras = run_quality_gate(name, result, MACRO_MODE)
                if gate_action == "RETRY":
                    print(f"  ⚠ Second retry still failing for {name}, proceeding anyway")
                    gate_action = "WARN"

            if gate_action == "HALT":
                print(f"  🛑 Gate HALT for {name}: {gate_msg}")
                pipeline_halted = True
                halt_message = f"{name}: {gate_msg}"
            elif gate_action == "WARN":
                print(f"  ⚠ Gate WARN for {name}: {gate_msg}")
            else:
                print(f"  ✅ Gate PASS for {name}")

            # Truncate excessive output (gate flagged it)
            max_chars = GATE_CONFIG.get("max_output_chars", 4000)
            if len(result) > max_chars:
                result = result[:max_chars] + "\n\n... [output truncated by factory gate] ..."
                print(f"  ✂ Output truncated for {name} ({len(result)} → {max_chars} chars)")

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
                    f"HPC Marketeer TODOs:\n{extract_todos_for('HPC Marketeer', result)}",
                ]
            )
        else:
            handoff_todos = f"Carry forward previous findings and TODOs. Latest output from {name}:\n{result[:1800]}"

    RUNS_DIR.mkdir(parents=True, exist_ok=True)
    PROPOSALS_DIR.mkdir(parents=True, exist_ok=True)

    # Extract and append new procedures discovered by agents
    procedures_path = TOWN_DIR / "procedures.md"
    new_procedures = []
    for name, _ in sequence:
        output = outputs.get(name, "")
        m = re.search(r"[78]\) Procedure Recording([\s\S]*?)(?:\n## |\Z)", output, flags=re.IGNORECASE)
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

    # --- Next-task preparation: agents recommend what to run next ---
    next_run_recs = extract_next_run(outputs)
    next_task_path = TOWN_DIR / "next-task.md"
    if next_run_recs and not pipeline_halted:
        next_task_content = (
            f"# Recommended Next Run\n\n"
            f"Generated by agent run `{run_id}` on {today}.\n\n"
            f"## Agent Recommendations\n\n"
            f"{next_run_recs}\n\n"
            f"## How to Execute\n\n"
            f"Pick the recommendation you agree with and run:\n\n"
            f"```fish\n"
            f"gh workflow run \"Agent Autonomous Run\" --ref dev-revert-main-linux \\\n"
            f"  -f macro=<Macro> -f task_command=\"<command from above>\"\n"
            f"```\n\n"
            f"Or use the GitHub Actions **Run workflow** button with the same values.\n"
        )
        next_task_path.write_text(next_task_content, encoding="utf-8")
        print(f"  📋 Next-task recommendations written to {next_task_path.relative_to(ROOT)}")
    elif next_task_path.exists():
        # Clear stale recommendations
        next_task_path.write_text(
            f"# Recommended Next Run\n\nNo follow-up recommended from run `{run_id}` ({today}).\n",
            encoding="utf-8",
        )

    report_path = RUNS_DIR / f"{today}-choice-a.md"
    proposal_md_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-proposal.md"
    proposal_patch_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-proposal.patch"
    guard_log_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-guard.log"

    report_lines = [
        f"# Choice A Auto Run — {today}",
        "",
        f"- Task: {task_id}",
        f"- Macro mode: {MACRO_MODE if MACRO_MODE else 'none'}",
        f"- Task command: {TASK_COMMAND if TASK_COMMAND else '(from current-task.md)'}",
        f"- Task mode: {TASK_MODE}",
        f"- Agent mode: {selected_agent if selected_agent else ('set:' + ', '.join(selected_set) if selected_set else 'all')}",
        f"- Auto apply patch: {'enabled' if AUTO_APPLY_PATCH else 'disabled'}",
        f"- Pipeline status: {'HALTED — ' + halt_message if pipeline_halted else 'completed'}",
        "- Sequence: " + " -> ".join([name for name, _ in sequence]),
        "",
    ]
    for name, _ in sequence:
        report_lines.append(f"## {name}")
        report_lines.append(outputs.get(name, ""))
        report_lines.append("")

    # Next-run recommendations in the report
    if next_run_recs:
        report_lines.append("## Recommended Next Run")
        report_lines.append("")
        report_lines.append(next_run_recs)
        report_lines.append("")

    # Factory metrics dashboard
    trailing = load_trailing_metrics(days=7)
    report_lines.append(compute_metrics_summary(trailing))
    report_lines.append("")

    proposal_lines = [
        f"# Code Proposal — {task_id} ({today})",
        "",
        "This file aggregates proposed code changes from the automated agent run.",
        "",
    ]
    for name, _ in sequence:
        proposal_lines.append(f"## {name}")
        m = re.search(r"6\) Code Proposal([\s\S]*)", outputs.get(name, ""), flags=re.IGNORECASE)
        proposal_lines.append(m.group(1).strip() if m else outputs.get(name, ""))
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
        "code_proposal": str(proposal_md_path.relative_to(ROOT)),
        "patch_proposal": str(proposal_patch_path.relative_to(ROOT)),
        "auto_apply_patch": AUTO_APPLY_PATCH,
        "apply_status": apply_status,
        "apply_message": apply_message,
        "pipeline_halted": pipeline_halted,
        "halt_message": halt_message if pipeline_halted else "",
        "allowed_scope_files": scope_rel,
        "agents": [name for name, _ in sequence],
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
