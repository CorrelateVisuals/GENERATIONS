#!/usr/bin/env python3
from __future__ import annotations

import datetime as dt
import json
import os
import re
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple
from urllib import request

ROOT = Path(os.environ.get("GITHUB_WORKSPACE", ".")).resolve()
AGENTS_DIR = ROOT / ".github" / "agents"
GUILDS_DIR = AGENTS_DIR / "guilds"
RUNS_DIR = AGENTS_DIR / "runs"
PROPOSALS_DIR = AGENTS_DIR / "proposals"

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
    "Party": [],
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
    procedures = read_text(AGENTS_DIR / "procedures.md")
    if procedures.strip():
        parts.append(procedures)
    return "\n\n".join(parts) if parts else ""


def call_llm(prompt: str) -> str:
    if LLM_PROVIDER == "openai":
        payload = {
            "model": LLM_MODEL,
            "messages": [{"role": "user", "content": prompt}],
            "temperature": 0.2,
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

    payload = {
        "model": LLM_MODEL,
        "messages": [{"role": "user", "content": prompt}],
        "temperature": 0.2,
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


def extract_todos_for(agent_name: str, text: str) -> str:
    pattern = rf"{re.escape(agent_name)}\s*TODOs?:([\s\S]*?)(?:\n\n|$)"
    m = re.search(pattern, text, flags=re.IGNORECASE)
    if not m:
        return "No explicit TODO block found; use previous agent findings as input."
    return m.group(1).strip()


MAX_CODE_CONTEXT = int(os.environ.get("MAX_CODE_CONTEXT", "12000"))


MACRO_DIRECTIVES: Dict[str, Dict[str, str]] = {
    "charge": {
        "C++ Lead": """## Macro Directive: CHARGE
You are the surgeon. Propose a MINIMAL patch for the command.
- Smallest viable diff. No exploration, no brainstorming.
- Be concrete: file paths, line-level changes, exact code.
- Mark each finding for cross-check by the next agent.""",
        "Vulkan Guru": """## Macro Directive: CHARGE (Validator)
You are the cross-check. Review the C++ Lead's proposal.
For EACH Lead finding, explicitly state one of:
- ✅ CONCUR — you independently reach the same conclusion
- ⚠️ QUALIFY — you agree but add constraints or caveats
- ❌ DISSENT — you disagree, provide evidence and alternative
End with: Combined confidence: HIGH / MEDIUM / LOW""",
    },
    "pull": {
        "C++ Lead": """## Macro Directive: PULL
You are the tank pulling the mob. MAP the terrain, do NOT propose patches.
- List every file touched, who owns it, and what depends on it.
- Estimate blast radius if this change goes wrong.
- Do NOT write code proposals — recon only.""",
        "Vulkan Guru": """## Macro Directive: PULL (Scout)
Add what the Lead MISSED in the Vulkan/resource domain.
- Confirm what Lead got right in your area.
- Add Vulkan-specific risks not visible from C++ level.
- Estimate YOUR domain's blast radius.
- Do NOT write code proposals — recon only.""",
        "Kernel Expert": """## Macro Directive: PULL (Scout)
Add what previous agents MISSED in the shader/GPU domain.
- Confirm what they got right about GPU impact.
- Add shader/dispatch/compute risks not visible from API level.
- Estimate YOUR domain's blast radius.
- Do NOT write code proposals — recon only.""",
        "Refactorer": """## Macro Directive: PULL (Scout)
Add what previous agents MISSED about structure and boundaries.
- Confirm architectural observations from prior agents.
- Assess complexity change: does this make the codebase simpler or harder?
- Recommend next macro: Charge (safe/small) or Follow (risky/large).
- Do NOT write code proposals — recon only.""",
    },
    "follow": {
        "C++ Lead": """## Macro Directive: FOLLOW
Full implementation cadence. You set direction for the entire party.
- Concrete scoped TODOs for every agent using Structured Handoff Format.
- Mark your key findings so downstream agents can CONCUR/QUALIFY/DISSENT.""",
        "Vulkan Guru": """## Macro Directive: FOLLOW
Do your Vulkan work AND cross-confirm the Lead's findings.
Include a ## Cross-Confirmation section:
- For each Lead finding relevant to you: ✅ CONCUR / ⚠️ QUALIFY / ❌ DISSENT
- State WHY you concur or dissent — evidence from code context.""",
        "Kernel Expert": """## Macro Directive: FOLLOW
Do your GPU/shader work AND cross-confirm prior agents.
Include a ## Cross-Confirmation section:
- vs C++ Lead: CONCUR/QUALIFY/DISSENT per finding
- vs Vulkan Guru: CONCUR/QUALIFY/DISSENT per finding""",
        "Refactorer": """## Macro Directive: FOLLOW
Do your structure work AND cross-confirm all prior agents.
Include a ## Cross-Confirmation section for Lead, Guru, and Expert.
- Flag any naming or boundary choices you challenge.""",
        "HPC Marketeer": """## Macro Directive: FOLLOW (Final)
Do your docs work AND produce the CONCURRENCE SUMMARY.
## Concurrence Summary
For each major finding, list which agents CONCUR/QUALIFY/DISSENT.
- 3+ concur → HIGH CONFIDENCE
- 2 agree, 1 dissents → NEEDS RESOLUTION
This is the definitive confidence matrix for the run.""",
    },
    "think": {
        "Vulkan Guru": """## Macro Directive: THINK (Independent)
You are theorycrafting ALONE. You do NOT see other agents' outputs.
Propose YOUR approach to this problem from a Vulkan/resource perspective.
- Approach description
- Affected files and scope
- Benefits and risks
- Complexity: S / M / L
- Recommended implementation sequence
Do NOT write patches. This is strategy, not execution.""",
        "Kernel Expert": """## Macro Directive: THINK (Independent)
You are theorycrafting ALONE. You do NOT see other agents' outputs.
Propose YOUR approach from a GPU/parallel/shader perspective.
- Approach description
- Affected files and scope
- Benefits and risks
- Complexity: S / M / L
- Recommended implementation sequence
Do NOT write patches. This is strategy, not execution.""",
        "Refactorer": """## Macro Directive: THINK (Independent)
You are theorycrafting ALONE. You do NOT see other agents' outputs.
Propose YOUR approach from an architecture/structure perspective.
- Approach description
- Affected files and scope
- Benefits and risks
- Complexity: S / M / L
- Recommended implementation sequence
Do NOT write patches. This is strategy, not execution.""",
    },
}


def get_macro_directive(macro_mode: str, agent_name: str) -> str:
    macro = (macro_mode or "").lower().strip()
    return MACRO_DIRECTIVES.get(macro, {}).get(agent_name, "")

def build_scoped_code(files: List[Path]) -> str:
    if not files:
        return "No in-scope files found."
    chunks = []
    total = 0
    for f in files:
        rel = f.relative_to(ROOT)
        snippet = read_capped(f)
        if total + len(snippet) > MAX_CODE_CONTEXT:
            remaining = MAX_CODE_CONTEXT - total
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
) -> str:
    prev_parts = []
    for name, txt in previous_outputs.items():
        trimmed = txt[:1500] if len(txt) > 1500 else txt
        prev_parts.append(f"## {name} Output\n{trimmed}")
    prev = "\n\n".join(prev_parts)
    incoming_trimmed = incoming_todos[:1500] if len(incoming_todos) > 1500 else incoming_todos
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

Return markdown with these exact sections:
1) Main Task Outcome
2) Secondary Task Outcomes
3) Risks and Constraints
4) Actionable TODOs (use Structured Handoff Format from base rules)
5) Handoff Note
6) Code Proposal
7) Cross-Confirmation (if macro directive requires it)
8) Procedure Recording (only if you discovered a new reusable pattern)
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
        "party": "Party",
        "party++": "Party",
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

    # Route macro mode to agent selection
    agent_only = AGENT_ONLY
    agent_set = AGENT_SET
    
    if MACRO_MODE:
        macro = MACRO_MODE.lower().strip()
        if macro == "charge":
            agent_only = ""
            agent_set = "C++ Lead,Vulkan Guru"
        elif macro in ("pull", "position"):
            agent_only = ""
            agent_set = "C++ Lead,Vulkan Guru,Kernel Expert,Refactorer"
        elif macro == "follow":
            agent_only = ""
            agent_set = "C++ Lead,Vulkan Guru,Kernel Expert,Refactorer,HPC Marketeer"
        elif macro == "think":
            agent_only = ""
            agent_set = "Vulkan Guru,Kernel Expert,Refactorer"
        elif macro == "party":
            agent_only = "Party"
            agent_set = ""
        elif macro == "lead++":
            agent_only = "C++ Lead"
            agent_set = ""
        elif macro == "agents++":
            agent_only = ""
            agent_set = "C++ Lead,Vulkan Guru,Kernel Expert,Refactorer,HPC Marketeer"

    base_md = read_text(AGENTS_DIR / "base.md")
    readme_md = read_text(AGENTS_DIR / "README.md")
    schedule_md = read_text(AGENTS_DIR / "schedule.md")
    task_md_main = read_text(AGENTS_DIR / "current-task.md")

    if TASK_MODE == "self":
        task_md = build_self_task(schedule_md, readme_md)
    else:
        task_md = task_md_main

    task_md = apply_task_command_override(task_md, TASK_COMMAND, MACRO_MODE)

    task_id = extract_task_id(task_md)
    scope_files = parse_scope_files(task_md)
    scope_rel = [str(p.relative_to(ROOT)) for p in scope_files]
    scoped_code = build_scoped_code(scope_files)

    all_agents: List[Tuple[str, str]] = [
        ("C++ Lead", "cpp-lead.md"),
        ("Vulkan Guru", "vulkan-guru.md"),
        ("Kernel Expert", "kernel-expert.md"),
        ("Refactorer", "refactorer.md"),
        ("HPC Marketeer", "hpc-marketeer.md"),
        ("Party", "party.md"),
    ]

    sequence: List[Tuple[str, str]] = [
        ("C++ Lead", "cpp-lead.md"),
        ("Vulkan Guru", "vulkan-guru.md"),
        ("Kernel Expert", "kernel-expert.md"),
        ("Refactorer", "refactorer.md"),
        ("HPC Marketeer", "hpc-marketeer.md"),
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

    if selected_agent == "Party":
        scoped_code = "Out of scope for Party agent by design. Party performs governance-only review over task framing and prior agent outputs."

    is_think = (MACRO_MODE or "").lower().strip() == "think"
    outputs: Dict[str, str] = {}
    handoff_todos = "Start from task statement; no upstream TODOs yet."

    for name, filename in sequence:
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
        result = call_llm(prompt)
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
    procedures_path = AGENTS_DIR / "procedures.md"
    new_procedures = []
    for name, _ in sequence:
        output = outputs.get(name, "")
        m = re.search(r"7\) Procedure Recording([\s\S]*?)(?:\n## |\Z)", output, flags=re.IGNORECASE)
        if m:
            proc_text = m.group(1).strip()
            if proc_text and "no new" not in proc_text.lower() and "none" not in proc_text.lower() and len(proc_text) > 20:
                new_procedures.append(f"\n### From {name} ({today}, {task_id})\n{proc_text}\n")
    if new_procedures:
        with open(procedures_path, "a", encoding="utf-8") as f:
            for proc in new_procedures:
                f.write(proc)

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
        "- Sequence: " + " -> ".join([name for name, _ in sequence]),
        "",
    ]
    for name, _ in sequence:
        report_lines.append(f"## {name}")
        report_lines.append(outputs.get(name, ""))
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

    patch_text = generate_patch(task_md, outputs, scope_rel)
    proposal_patch_path.write_text(patch_text, encoding="utf-8")

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
        "allowed_scope_files": scope_rel,
        "agents": [name for name, _ in sequence],
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
