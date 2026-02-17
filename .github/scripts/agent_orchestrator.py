#!/usr/bin/env python3
from __future__ import annotations

import datetime as dt
import json
import os
import re
from pathlib import Path
from typing import Dict, List, Tuple
from urllib import request

ROOT = Path(os.environ.get("GITHUB_WORKSPACE", ".")).resolve()
AGENTS_DIR = ROOT / ".github" / "agents"
PARTY_DIR = AGENTS_DIR / "party"
TOWN_DIR = AGENTS_DIR / "town"
RUNS_DIR = AGENTS_DIR / "runs"
PROPOSALS_DIR = AGENTS_DIR / "proposals"

LLM_API_URL = os.environ.get("LLM_API_URL", "https://api.openai.com/v1/responses")
LLM_MODEL = os.environ.get("LLM_MODEL", "gpt-4.1")
LLM_API_KEY = os.environ.get("LLM_API_KEY", "")

if not LLM_API_KEY:
    raise SystemExit("Missing LLM_API_KEY secret/env.")


def read_text(path: Path, default: str = "") -> str:
    if not path.exists():
        return default
    return path.read_text(encoding="utf-8")


def read_capped(path: Path, max_chars: int = 12000) -> str:
    content = read_text(path)
    if len(content) <= max_chars:
        return content
    head = content[: max_chars // 2]
    tail = content[-max_chars // 2 :]
    return f"{head}\n\n... [truncated] ...\n\n{tail}"


def parse_scope_files(task_md: str) -> List[Path]:
    candidates: List[Path] = []
    for line in task_md.splitlines():
        line = line.strip()
        if line.startswith("-") and "`" in line:
            matches = re.findall(r"`([^`]+)`", line)
            for m in matches:
                if "*" in m:
                    base = m.split("*")[0]
                    base_path = ROOT / base
                    for hit in sorted(ROOT.glob(m)):
                        if hit.is_file():
                            candidates.append(hit)
                    if not any(p for p in candidates if str(p).startswith(str(base_path))):
                        continue
                else:
                    p = ROOT / m
                    if p.exists() and p.is_file():
                        candidates.append(p)
    unique: List[Path] = []
    seen = set()
    for c in candidates:
        key = str(c)
        if key in seen:
            continue
        seen.add(key)
        unique.append(c)
    return unique


def call_llm(prompt: str) -> str:
    payload = {
        "model": LLM_MODEL,
        "input": prompt,
        "temperature": 0.2,
    }
    data = json.dumps(payload).encode("utf-8")
    req = request.Request(
        LLM_API_URL,
        data=data,
        headers={
            "Authorization": f"Bearer {LLM_API_KEY}",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    with request.urlopen(req, timeout=120) as resp:
        raw = resp.read().decode("utf-8")
    body = json.loads(raw)

    if isinstance(body, dict):
        if "output_text" in body and body["output_text"]:
            return body["output_text"]
        output = body.get("output", [])
        texts: List[str] = []
        for item in output:
            if item.get("type") != "message":
                continue
            for content in item.get("content", []):
                if content.get("type") == "output_text":
                    texts.append(content.get("text", ""))
        if texts:
            return "\n".join(texts)
    return raw


def extract_task_id(task_md: str) -> str:
    m = re.search(r"## Task ID\s*\n\s*([A-Za-z0-9_-]+)", task_md)
    return m.group(1) if m else "TASK"


def extract_todos_for(agent_name: str, text: str) -> str:
    pattern = rf"{re.escape(agent_name)}\s*TODOs?:([\s\S]*?)(?:\n\n|$)"
    m = re.search(pattern, text, flags=re.IGNORECASE)
    if not m:
        return "No explicit TODO block found; use previous agent findings as input."
    return m.group(1).strip()


def build_prompt(
    base_md: str,
    profile_md: str,
    task_md: str,
    scoped_code: str,
    previous_outputs: Dict[str, str],
    incoming_todos: str,
    agent_name: str,
) -> str:
    prev = "\n\n".join([f"## {k} Output\n{v}" for k, v in previous_outputs.items()])
    return f"""
You are running inside the GENERATIONS multi-agent pipeline.

Use these instructions as strict context:

# Shared Base
{base_md}

# Agent Profile
{profile_md}

# Active Task
{task_md}

# Incoming Handoff TODOs for {agent_name}
{incoming_todos}

# Relevant Code Context
{scoped_code}

# Previous Agent Outputs
{prev if prev else 'None'}

Return markdown with these exact sections:
1) Main Task Outcome
2) Secondary Task Outcomes
3) Risks and Constraints
4) Actionable TODOs
5) Handoff Note
6) Code Proposal

Rules for Code Proposal:
- Provide concrete, file-targeted proposals.
- Prefer minimal changes.
- If uncertain, provide draft patch blocks and clearly label assumptions.
""".strip()


def main() -> None:
    today = dt.date.today().isoformat()
    base_md = read_text(TOWN_DIR / "base.md")
    readme_md = read_text(TOWN_DIR / "README.md")
    task_md = read_text(TOWN_DIR / "current-task.md")
    task_id = extract_task_id(task_md)

    files = parse_scope_files(task_md)
    scoped_parts = []
    for f in files:
        rel = f.relative_to(ROOT)
        scoped_parts.append(f"## {rel}\n{read_capped(f)}")
    scoped_code = "\n\n".join(scoped_parts) if scoped_parts else "No in-scope files found."

    sequence: List[Tuple[str, str]] = [
        ("C++ Lead", "party/cpp-lead.md"),
        ("Vulkan Guru", "party/vulkan-guru.md"),
        ("Kernel Expert", "party/kernel-expert.md"),
        ("Refactorer", "party/refactorer.md"),
        ("HPC Marketeer", "party/hpc-marketeer.md"),
    ]

    outputs: Dict[str, str] = {}
    handoff_todos = "Start from task statement; no upstream TODOs yet."

    for name, filename in sequence:
        profile_md = read_text(AGENTS_DIR / filename)
        prompt = build_prompt(
            base_md=base_md,
            profile_md=profile_md,
            task_md=task_md,
            scoped_code=scoped_code,
            previous_outputs=outputs,
            incoming_todos=handoff_todos,
            agent_name=name,
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
            handoff_todos = f"Carry forward previous findings and TODOs. Latest output from {name}:\n{result[:2000]}"

    RUNS_DIR.mkdir(parents=True, exist_ok=True)
    PROPOSALS_DIR.mkdir(parents=True, exist_ok=True)

    report_path = RUNS_DIR / f"{today}-choice-a.md"
    proposal_path = PROPOSALS_DIR / f"{today}-{task_id.lower()}-proposal.md"

    report_lines = [
        f"# Choice A Auto Run — {today}",
        "",
        f"- Task: {task_id}",
        "- Mode: Automated sequential agent orchestration",
        "- Sequence: C++ Lead -> Vulkan Guru -> Kernel Expert -> Refactorer -> HPC Marketeer",
        "",
    ]
    for name, _ in sequence:
        report_lines.append(f"## {name}")
        report_lines.append(outputs.get(name, ""))
        report_lines.append("")

    report_path.write_text("\n".join(report_lines).strip() + "\n", encoding="utf-8")

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

    proposal_path.write_text("\n".join(proposal_lines).strip() + "\n", encoding="utf-8")

    summary = {
        "task": task_id,
        "run_report": str(report_path.relative_to(ROOT)),
        "code_proposal": str(proposal_path.relative_to(ROOT)),
        "agents": [name for name, _ in sequence],
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
