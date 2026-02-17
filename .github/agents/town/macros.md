# Agent Macros (WoW Multibox Style)

> **Authoritative definitions live in [`macros.json`](macros.json).**
> This file is human-readable documentation only. If they diverge, the JSON wins.

This file defines macro-style control for the 5-agent team using one-driver command syntax:

`<Macro> <Command>`

Example:
- `Charge "Swapchain recreation"`
- `Pull "Buffer creation"`
- `Follow "Split base classes"`
- `Think "How to approach duplicate buffers in class X"`

## Party Mapping (Warrior Comp)

- Warrior Tank → **C++ Lead** (main tank, first contact, threat/priority owner)
- Mage → **Vulkan Guru** (specialized burst, precision resource control)
- Hunter → **Kernel Expert** (steady ranged throughput on GPU/parallel paths)
- Shaman → **Refactorer** (utility, cleanup, totems = architectural foundations)
- Priest Healer → **HPC Marketeer** (stability, clarity, keeps the party healthy)

## Governance Override

- **Party** → governance-only controller for agent alignment
- Scope: checks contradictions, overlap, missing ownership, and integration risk
- Constraint: Party does **not** analyze source code directly

## Macro Definitions

---

### 1) `Charge <Command>`

> *Warrior charges in — fast, decisive, minimal.*

**What it is:** Surgical strike with one cross-check.
**What it is NOT:** Exploration or brainstorming.

**Execution**
1. `C++ Lead` → proposes minimal patch for the command.
2. `Vulkan Guru` → validates from resource/sync perspective.

**Cross-Confirmation**
Guru must mark each Lead finding:
- ✅ **CONCUR** — independently agree
- ⚠️ **QUALIFY** — agree with caveats
- ❌ **DISSENT** — disagree with evidence

**Output:** Patch + confidence level (HIGH/MEDIUM/LOW).

**When to use:** You know what you want. Just do it, but don't break Vulkan.

---

### 2) `Pull <Command>`

> *Tank pulls the mob — watch how it reacts before committing the party.*

**What it is:** Sequential recon. Each specialist scouts the same target from their angle, building on what the previous scout reported.
**What it is NOT:** Implementation. No patches, no code proposals.

**Execution (sequential — each sees previous output)**
1. `C++ Lead` → maps dependencies, ownership, affected files, blast radius.
2. `Vulkan Guru` → adds: Vulkan resource/sync risks the Lead missed.
3. `Kernel Expert` → adds: shader/dispatch/GPU risks the others missed.
4. `Refactorer` → adds: structural/boundary risks, complexity assessment.

**Cross-Confirmation Rule**
Each specialist after Lead answers:
- What did previous agents **miss** in my domain?
- What do I **independently confirm**?
- What is my domain's **blast radius** if this goes wrong?

**Output:** Dependency map + layered risk assessment + recommended next macro.

**When to use:** You're unsure of the scope or impact. Pull first, then Charge or Follow.

---

### 3) `Follow <Command>`

> *Full party /follow — everyone moves together, tank leads.*

**What it is:** Full implementation cadence. All 5 agents, sequential, each validates the previous AND adds their own work.
**What it is NOT:** Quick. This is the full raid.

**Execution Cadence**
1. `C++ Lead` → direction + scoped TODOs for all agents.
2. `Vulkan Guru` → Vulkan work + confirms/challenges Lead's resource assumptions.
3. `Kernel Expert` → GPU work + confirms/challenges Guru's barrier decisions.
4. `Refactorer` → structure work + confirms/challenges all prior boundary choices.
5. `HPC Marketeer` → docs + **Concurrence Summary** across all agents.

**Cross-Confirmation (cascading)**
Each agent includes `## Cross-Confirmation` with CONCUR/QUALIFY/DISSENT markers for every prior agent.

**Concurrence Tracking**
- 3+ agents flag same issue → **HIGH CONFIDENCE**
- 2 agree, 1 dissents → **NEEDS RESOLUTION**
- HPC Marketeer produces the final concurrence matrix.

**Output:** Full proposal + phased plan + cross-confirmation matrix + patch.

**When to use:** You want the full treatment. Architecture to docs to patch.

---

### 4) `Think <Command>`

> *Theorycrafting in trade chat — each class champion argues their own strat.*

**What it is:** Three specialists work the SAME problem INDEPENDENTLY. They don't see each other's output. You compare afterwards.
**What it is NOT:** Sequential collaboration. This is deliberate isolation for honest divergence.

**Key difference from Pull:**
| | Pull | Think |
|---|---|---|
| Mode | Sequential (builds on previous) | Independent (isolated) |
| Goal | Map the terrain | Compete on strategy |
| Sees others | Yes, adds to picture | No, works alone |
| Output | Layered risk map | Three competing approaches |
| Patches | No | No |

**Execution (independent — each gets ONLY base context)**
1. `Vulkan Guru` → Vulkan/resource-first approach (alone)
2. `Kernel Expert` → GPU/parallel-first approach (alone)
3. `Refactorer` → architecture/structure-first approach (alone)

**Independence Rule**
Each agent gets the same code context and task but does NOT see other agents' outputs. This ensures genuine independent analysis.

**Output per agent:**
- Approach description
- Affected files and scope
- Benefits and risks
- Complexity: S / M / L
- Recommended implementation sequence

**Output summary (generated post-run):**
- **Convergence points** — where 2+ agents independently reached the same conclusion (= high confidence)
- **Divergence points** — where agents disagree (= needs debate)
- Recommended next macro: `Charge` (convergence) or `Follow` (divergence)

**When to use:** You don't know the right approach yet. Let the specialists argue independently, then see where they agree.

---

## Macro Flow

```
Think ──→ found convergence? ──→ Charge (fast execute)
  │                                    
  └──→ found divergence? ──→ Pull (scout deeper) ──→ Follow (full implement)
```

## CLI Usage

```fish
# Remote (GitHub Actions)
gh workflow run "Agent Autonomous Run" --ref dev-revert-main-linux \
  -f macro=Charge -f task_command="swapchain recreation"

gh workflow run "Agent Autonomous Run" --ref dev-revert-main-linux \
  -f macro=Pull -f task_command="buffer creation"

gh workflow run "Agent Autonomous Run" --ref dev-revert-main-linux \
  -f macro=Follow -f task_command="split base classes"

gh workflow run "Agent Autonomous Run" --ref dev-revert-main-linux \
  -f macro=Think -f task_command="how to approach duplicate buffers"
```

## Task Sizing Rule

Per run target:
- 1 clear objective
- 2-8 files in scope
- ~50-400 changed lines

If bigger, split into phases:
- Phase A: architecture/interface
- Phase B: implementation
- Phase C: docs/cleanup