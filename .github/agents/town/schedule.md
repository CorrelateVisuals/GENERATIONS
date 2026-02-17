# Agent Tasks & Schedules

This file centralizes recurring agent responsibilities and gives schedule options you can choose from.

## Active Selection

- Active baseline: **Choice A — Balanced**
- Effective from: **2026-02-17**
- Report location: **`.github/agents/runs/`**
- Code change location: **feature branches + Pull Requests**
- Automated run windows:
  - **Friday 05:00 Amsterdam**
  - **Sunday 21:00 Amsterdam**
  - **In-between autonomous tasks**: Tuesday + Thursday

## Agent Task Catalog

| Agent | Recurring Tasks |
|---|---|
| C++ Lead | Review unsafe patterns, lifecycle ownership, error handling paths, cross-platform compile-impact |
| Vulkan Guru | Review descriptor/pipeline reuse, synchronization correctness, image/layout transitions, validation warnings |
| Kernel Expert | Review compute dispatch dimensions, shader IO/layout alignment, GPU-bound opportunities, shader perf hotspots |
| Refactorer | Review naming/clarity, architectural boundaries, dead code removal opportunities, complexity hotspots |
| HPC Marketeer | Review README/setup accuracy, docs drift from implementation, onboarding friction, release notes clarity |
| Guild Master | Weekly class performance assessment, class change directives, guild policy directives, conflict arbitration |

## Guild Master Weekly Assessment

The Guild Master runs automatically every **Friday** (same window as HPC Marketeer synthesis).

### What the Guild Master Reviews
1. **Factory metrics** — trailing 14-day `metrics.jsonl` data (last 30 records raw)
2. **Agent profiles** — current class definitions in `party/*.md` (full content)
3. **Guild membership map** — AGENT_GUILDS assignments
4. **Guild policies** — current guild files in `guilds/*.md` (full content)
5. **Procedures** — living patterns in `town/procedures.md` (full content)
6. **Quality gate config** — current thresholds and flags
7. **Governance log** — previous Guild Master decisions in `guilds/governance-log.md`
8. **Recent run reports** — last 3 run reports from `runs/*.md`

### What the Guild Master Produces
- **Class Performance Report** — per-agent scorecard (gate pass rate, retry rate, dissent ratio)
- **Class Change Directives** — auto-logged to `guilds/governance-log.md` (profile adjustments, guild reassignments)
- **Guild Policy Directives** — auto-applied to `procedures.md` and logged to `guilds/governance-log.md`
- **Arbitration Decisions** — resolves conflicting agent recommendations
- **Risk Assessment** — integration risks from mismatched recommendations

### Trigger
- Automatic: Friday scheduled run (macro: Guild)
- Manual: `gh workflow run agent-autonomous-run.yml --ref <branch> -f macro=Guild`

## Schedule Choices

Pick one of these presets as your team baseline.

### Choice A — Balanced (recommended)
- **Daily (Mon–Fri)**
  - C++ Lead (20 min)
  - Vulkan Guru (20 min)
  - Kernel Expert (20 min)
- **Twice per week (Tue/Thu)**
  - Refactorer (30 min)
- **Weekly (Fri)**
  - HPC Marketeer (30 min)
- **Weekly synthesis (Fri)**
  - 15-min handoff summary with top 3 findings + actions

### Test Run (Choice A)

- **Pilot day**: first Friday after activation, then weekly on Fridays.
- **Execution window**: 45–60 minutes total.
- **Expected output per agent**:
  1. Top findings (max 5)
  2. Risk level
  3. Proposed actions
  4. Target files/PR
- **Aggregation**: one weekly summary file in `.github/agents/runs/`.

### Autonomous Runtime Notes

- GitHub schedules run in UTC; workflow uses winter/summer UTC pairs to track Amsterdam local targets.
- In-between runs execute in **self-task mode** and may select maintenance actions unrelated to the current manual task.

### Where Results and Changes Are Found

- **Agent findings**: markdown files in `.github/agents/runs/`
- **Proposed/implemented code changes**: Pull Requests linked from that weekly run file
- **Discussion trail**: PR comments/reviews per agent perspective
- **Final accepted changes**: merged commits on `main`

### Choice B — Performance Sprint
- **Daily (Mon–Fri)**
  - Vulkan Guru (30 min)
  - Kernel Expert (30 min)
- **Every other day (Mon/Wed/Fri)**
  - C++ Lead (20 min)
- **Weekly (Thu)**
  - Refactorer (20 min)
- **Weekly (Fri)**
  - HPC Marketeer (20 min, docs limited to changed performance behavior)

### Choice C — Stability & Cleanup
- **Daily (Mon–Fri)**
  - C++ Lead (25 min)
  - Refactorer (25 min)
- **Twice per week (Tue/Thu)**
  - Vulkan Guru (20 min)
- **Weekly (Wed)**
  - Kernel Expert (20 min)
- **Weekly (Fri)**
  - HPC Marketeer (30 min)

## Suggested Rotation Format

For each scheduled run, capture:
1. **Findings** (max 5 bullets)
2. **Risk level** (Low/Med/High)
3. **Action owner**
4. **Target PR or file set**
5. **Carry-over items**

## Lightweight Weekly Template

```md
## Week of YYYY-MM-DD
- Chosen schedule: A | B | C
- Focus theme: <performance | stability | docs | feature>

### C++ Lead
- Findings:
- Actions: 

### Vulkan Guru
- Findings:
- Actions:

### Kernel Expert
- Findings:
- Actions:

### Refactorer
- Findings:
- Actions:

### HPC Marketeer
- Findings:
- Actions:
```
