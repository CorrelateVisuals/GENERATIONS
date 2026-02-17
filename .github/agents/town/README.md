# GENERATIONS Development Agents

This directory contains agent profiles that define specialized roles for development of the GENERATIONS engine. Each agent represents a unique perspective and skill set designed to ensure comprehensive coverage of all aspects of the codebase.

## Purpose

These archetype agents are designed to be rotated during development and code review to ensure:
- No architectural blind spots
- Balanced consideration of performance, correctness, maintainability, and efficiency
- Comprehensive expertise across C++, Vulkan, GLSL, software architecture, and user experience
- Clear, accessible documentation that stays synchronized with code

## Operating Protocol

- Use `base.md` for shared rules to avoid duplicated instructions in each profile.
- Agents are driven by a manually provided task/action.
- Active manual task is documented in `current-task.md`.
- Agents run in sequence:
	1. `C++ Lead` (mandatory first pass)
	2. `Vulkan Guru`
	3. `Kernel Expert`
	4. `Refactorer`
	5. `HPC Marketeer`
- `C++ Lead` must produce TODO handoffs for all following agents based on the current task.
- Every agent responds with: main task outcome, secondary outcomes, risks, TODOs, and handoff note.

## Automation (optional)

- Autonomous scheduled execution is defined in `.github/workflows/agent-autonomous-run.yml`.
- Runner script: `.github/scripts/agent_autorun.py`.
- Required repository secret:
	- `LLM_API_KEY`
- Optional repository variables:
	- `LLM_MODEL` (default: `gpt-4.1`)
	- `LLM_API_URL` (default: `https://api.openai.com/v1/responses`)
	- `LLM_PROVIDER` (default: `auto`, set to `github` for GitHub Models)
- Optional guard variables:
	- `MAX_PATCH_FILES` (default: `8`)
	- `MAX_PATCH_LINES` (default: `400`)
	- `GUARD_BUILD_CMD` (default: empty)
- Automation output:
	- Run reports in `.github/agents/runs/`
	- Code proposals in `.github/agents/proposals/`
	- Auto PR containing generated artifacts

### Schedule Windows

- Main-task runs (Amsterdam target):
	- Friday 05:00 local (handled with UTC winter/summer pair)
	- Sunday 21:00 local (handled with UTC winter/summer pair)
- In-between autonomous maintenance runs:
	- Tuesday and Thursday (self-task mode)

### Guarded Auto-Apply Mode

- Workflow can auto-apply generated patch proposals before creating PR.
- Guard checks:
	- changed files limited to in-scope task files
	- max changed files and max changed lines
	- optional build command gate (`GUARD_BUILD_CMD`)
- If any guard fails, patch is rejected and workflow falls back to proposal artifacts only.

Note: automation can run in proposal-only mode or guarded auto-apply mode, depending on workflow input/schedule.

### GitHub Models Support

The runner supports GitHub Models via `GITHUB_TOKEN` when:
- `LLM_PROVIDER=github`, or
- `LLM_PROVIDER=auto` and `GITHUB_TOKEN` is set

When using GitHub Models, also set:
- `LLM_MODEL` to a GitHub Models ID (for example, `gpt-4o-mini`)
- `LLM_API_URL` if you want to override the default endpoint

### Macro Support in GitHub Actions

The workflow now supports macro-style execution via workflow_dispatch input:
- Go to Actions → "Agent Autonomous Run" → Run workflow
- Select macro from dropdown: `Charge`, `Pull`, `Follow`, `Think`, `Guild`, `lead++`, `agents++`
- Leave empty for full 5-agent sequence

This enables the same macro control from the GitHub web UI that local scripts provide.

### Local Macro Commands

- `./lead++` runs only `C++ Lead` (uses `AGENT_ONLY="C++ Lead"`).
- `./agents++` runs the full five-agent sequence.
- `./Guild` runs only `Guild Master` (weekly class assessment, policy proposals; no code analysis).
- `./Charge "<Command>"` runs C++ Lead surgical execution for one command.
- `./Pull "<Command>"` runs sequential recon pass (Lead, Guru, Expert, Refactorer).
- `./Follow "<Command>"` runs full-cadence proposal pass for one command.
- `./Think "<Command>"` runs independent theorycrafting (Guru, Expert, Refactorer in isolation).
- Both commands print JSON containing links/paths to run report + proposal artifacts.
- Additional guidance: `.github/agents/town/macros.md`.

## The Five Archetypes

### 1. [C++ Lead](../party/cpp-lead.md)
**Focus**: Real-time safety-critical systems engineering

The C++ Lead ensures that code is reliable, efficient, and maintainable. This agent brings modern C++ expertise with a focus on understanding the consequences of low-level implementation decisions. Elegant, readable solutions are preferred over unnecessary additions.

**Key question**: "Is this the simplest, safest, most maintainable C++ implementation?"

### 2. [Vulkan Guru](../party/vulkan-guru.md)
**Focus**: Efficient and precise Vulkan API usage

The Vulkan Guru maximizes the potential of the Vulkan API by reusing existing resources and inserting precision solutions. This agent sees possibilities for leveraging Vulkan features and prefers automation of common patterns.

**Key question**: "Are we using Vulkan resources and features as efficiently as possible?"

### 3. [Kernel Expert](../party/kernel-expert.md)
**Focus**: GPU-first parallel computing and shader development

The Kernel Expert thinks in parallel and speaks GLSL fluently. This agent sees shader inputs and outputs clearly and believes that if something can run on the GPU, it should. Creative and brilliant shader solutions are this agent's specialty.

**Key question**: "Can this be done more efficiently on the GPU?"

### 4. [Refactorer](../party/refactorer.md)
**Focus**: Code quality, maintainability, and architectural integrity

The Refactorer ensures code is clean, readable, adheres to established standards, and follows MVC principles. Less code is better, and long-term usability of the codebase is the primary concern.

**Key question**: "Is this code clear, maintainable, and properly architected for the long term?"

### 5. [HPC Marketeer](../party/hpc-marketeer.md)
**Focus**: Documentation and user experience

The HPC Marketeer understands what users want and identifies what's cumbersome or hard to understand. Expert at presenting software effectively and maintaining documentation to support it. README.md is home base, but extends to all repository documentation. Keeps docs clean so they're easy to update when code changes.

**Key question**: "Will users understand this, and is the documentation accurate and up-to-date?"

## Coverage Matrix

Together, these agents ensure comprehensive coverage:

| Aspect | Primary Agent | Supporting Agents |
|--------|--------------|-------------------|
| C++ code quality | C++ Lead | Refactorer |
| Vulkan API usage | Vulkan Guru | C++ Lead |
| Shader development | Kernel Expert | Vulkan Guru |
| Architecture boundaries | Refactorer | C++ Lead |
| Performance (CPU) | C++ Lead | Refactorer |
| Performance (GPU) | Kernel Expert | Vulkan Guru |
| Resource management | Vulkan Guru | C++ Lead |
| Code maintainability | Refactorer | All |
| Long-term design | Refactorer | C++ Lead |
| Documentation quality | HPC Marketeer | Refactorer |
| User experience | HPC Marketeer | All |
| Onboarding & setup | HPC Marketeer | C++ Lead |

## How to Use These Agents

When reviewing code or planning changes:

1. Define one manual task statement.
2. Run `C++ Lead` first and collect handoff TODOs.
3. Run remaining agents in sequence using the same task plus their handed TODOs.
4. Aggregate outcomes into one run report in `.github/agents/runs/`.
5. Open or update PRs using the aggregated TODOs.

## Practical Application

**Example workflow for a new feature**:
1. **C++ Lead**: Review implementation for safety, efficiency, and C++ best practices
2. **Vulkan Guru**: Optimize Vulkan resource usage and API patterns
3. **Kernel Expert**: Move appropriate work to GPU shaders
4. **Refactorer**: Ensure code is clean, well-documented, and architecturally sound
5. **HPC Marketeer**: Update documentation, validate user-facing changes are clear

This multi-perspective approach ensures the GENERATIONS engine remains reliable, high-performance, maintainable, and accessible.

## Governance Agent

### [Guild Master](../guilds/guild-master.md)
**Focus**: Class performance assessment, guild policy, and conflict arbitration

The Guild Master reviews Factory metrics to assess how each agent class is performing. It proposes class changes (agent profile adjustments, guild reassignments) and guild policy changes (procedure updates, gate threshold tuning). It does not inspect source code directly.

**Key question**: "Are the agents performing well, and what class or policy changes would improve the pipeline?"
