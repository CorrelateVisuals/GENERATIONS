# Agent Macros (WoW Multibox Style)

This file defines macro-style control for the 5-agent team using one-driver command syntax:

`<Macro> <Command>`

Example:
- `Charge "Swapchain recreation"`
- `Position "Buffer creation"`
- `Follow "Split base classes"`
- `Think "How to approach duplicate buffers in class X"`

Executable commands now available at repository root:
- `./Charge "<Command>"`
- `./Position "<Command>"`
- `./Follow "<Command>"`
- `./Think "<Command>"`

## Party Mapping (Warrior Comp)

- Warrior Tank -> **C++ Lead** (primary direction, first contact, threat/priority owner)
- Priest Healer -> **HPC Marketeer** (stability, clarity, recovery/docs)
- Shaman -> **Refactorer** (utility, cleanup, consistency buffs)
- Mage -> **Vulkan Guru** (specialized burst optimization)
- Hunter -> **Kernel Expert** (steady ranged throughput on GPU/parallel paths)

## Governance Override

- **Party** -> governance-only controller for agent alignment
- Scope: checks contradictions, overlap, missing ownership, and integration risk across agent outputs
- Constraint: Party does **not** analyze source code directly

## Macro Definitions

### 1) `Charge <Command>`

**Purpose**
- Surgical execution for one concrete intent.
- Minimal insert/change set to achieve the desired result.

**Execution**
- Calls: `C++ Lead` first.
- Optional specialist call only if C++ Lead flags a hard dependency.
- Patch budget: smallest viable diff.

**Expected Output**
- Direct proposal with minimal-file patch.
- Risk note and rollback hint.

### 2) `Position <Command>`

**Purpose**
- Awareness and dependency mapping before implementation.
- Identify what code is tied to the command and possible simplifications.

**Execution**
- Always starts with `C++ Lead`.
- Only calls agents with direct relation to topic:
  - Vulkan concerns -> `Vulkan Guru`
  - Shader/compute concerns -> `Kernel Expert`
  - Structural/maintainability concerns -> `Refactorer`
  - Documentation/onboarding concerns -> `HPC Marketeer`
- Non-called agents are explicitly listed as "not involved".

**Expected Output**
- Relationship map (what is tied in).
- Simplification suggestions.
- Recommended next macro (`Charge` or `Follow`).

### 3) `Follow <Command>`

**Purpose**
- Full follow-through of an idea from architecture to implementation proposal.

**Execution Cadence**
1. `C++ Lead` -> defines direction + creates TODOs for all others.
2. `Vulkan Guru` -> Vulkan/resource correctness and optimization impact.
3. `Kernel Expert` -> GPU/parallel path impact.
4. `Refactorer` -> structure, cleanup, phased migration.
5. `HPC Marketeer` -> clarity/docs/release-facing summary.

**Expected Output**
- Full proposal with phased plan.
- Consolidated TODO stack by agent.
- Proposal patch artifact + PR-ready summary.

### 4) `Think <Command>`

**Purpose**
- Generate multiple specialist approaches before deciding execution.
- Compare trade-offs without committing to a patch immediately.

**Execution**
- Calls exactly these three agents:
  - `Refactorer`
  - `Vulkan Guru`
  - `Kernel Expert`
- `C++ Lead` is not mandatory in this macro pass.
- Each called agent must provide one unique approach to the same command.

**Expected Output**
- Three distinct approaches:
  1. Refactor/architecture-first approach (`Refactorer`)
  2. Vulkan/resource-first approach (`Vulkan Guru`)
  3. GPU/parallel-path-first approach (`Kernel Expert`)
- For each approach: scope, benefits, risks, and complexity estimate.
- A short recommendation for which macro to run next (`Charge` or `Follow`).

## Local Command Mapping

Current local wrappers map like this:
- `./lead++` -> closest to `Charge` first pass (lead-only)
- `./agents++` -> closest to `Follow` full cadence
- `./Party` -> governance-only consistency/arbitration pass
- `Think "<command>"` -> tri-specialist ideation pass (Refactorer + Vulkan Guru + Kernel Expert)
- `./Charge "<command>"` -> implemented macro command (C++ Lead surgical pass)
- `./Position "<command>"` -> implemented macro command (relationship-aware selective agents)
- `./Follow "<command>"` -> implemented macro command (full 5-agent cadence)
- `./Think "<command>"` -> implemented tri-specialist ideation pass

If needed later, add wrapper alias:
- `./think++ "<command>"` -> alias to `./Think`

## Token Setup (GitHub Models)

Local runs can use a GitHub token instead of an OpenAI key:
- `set -Ux GITHUB_TOKEN "<your_token>"` (fish)
- `export GITHUB_TOKEN="<your_token>"` (bash/zsh)

Set model/provider for GitHub Models:
- `set -Ux LLM_PROVIDER github`
- `set -Ux LLM_MODEL gpt-4o-mini`

## Task Sizing Rule

Per run target:
- 1 clear objective
- 2-8 files in scope
- ~50-400 changed lines

If bigger, split into phases:
- Phase A: architecture/interface
- Phase B: implementation
- Phase C: docs/cleanup