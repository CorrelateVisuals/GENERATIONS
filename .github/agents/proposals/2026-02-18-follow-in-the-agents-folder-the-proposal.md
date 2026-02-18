# Code Proposal — FOLLOW-IN-THE-AGENTS-FOLDER-THE (2026-02-18)

This file aggregates proposed code changes from the automated agent run.

## C++ Lead
```markdown
# Proposed Updates to `.github/agents/town/prompt-catalog.md`

## General Guidelines
- Ensure all prompts are specific, actionable, and aligned with the shared mission.
- Use structured handoff format for all TODOs.
- Follow guild procedures for task patterns.

## Example Prompt Updates
### C++ Lead
- "Analyze the task and define handoff TODOs for all other agents. Ensure recommendations are concrete and file-scoped."
- "Focus on runtime optimization and architectural boundaries. Avoid speculative abstractions."

### Vulkan Guru
- "Validate Vulkan resource semantics and synchronization. Ensure alignment with GPU Pipeline Guild procedures."
- "Review Vulkan-specific prompts for clarity and correctness."

### Kernel Expert
- "Optimize GPU utilization and shader performance. Follow PROC-PERF-001 and PROC-PERF-002."
- "Ensure compatibility between shader and dispatch configurations."

### Refactorer
- "Refine naming and structure for clarity and consistency. Validate against architectural boundaries."
- "Ensure prompt updates reduce complexity and improve maintainability."

### HPC Marketeer
- "Update documentation to reflect prompt catalog changes. Ensure alignment with user-facing materials."
- "Add examples and workflows to improve onboarding experience."

## 7) Cross-Confirmation
No prior agent findings to CONCUR/QUALIFY/DISSENT as this is the first agent in the sequence.

## 8) Procedure Recording
### PROC-ARCH-002 — Prompt Catalog Refinement
1. Identify areas in the prompt catalog that require updates for clarity and alignment.
2. Propose a modular structure for the catalog to improve maintainability.
3. Use structured handoff format for all TODOs.
4. Synchr

... [output truncated by factory gate] ...

## Vulkan Guru
No code changes are required for this task, as it focuses on refining the prompt catalog. However, the following structure is proposed for Vulkan-related prompts in `.github/agents/town/prompt-catalog.md`:

```markdown
### Vulkan Guru Prompts
- **Task Context**: Clearly define the Vulkan-specific task, including relevant files and responsibilities.
- **Actionable Steps**: Provide a step-by-step guide for completing the task, ensuring alignment with Vulkan Guru's decision-making principles.
- **Validation Criteria**: Specify how the task's success will be validated (e.g., validation layers, performance profiling).
- **Cross-Agent Coordination**: Highlight any dependencies or handoffs to other agents (e.g., Kernel Expert, Refactorer).
```

## 7) Cross-Confirmation
### C++ Lead Findings
1. **Ambiguity in prompts**  
   ✅ CONCUR — Ambiguity in prompts can lead to inconsistent agent behavior. Ensuring prompts are specific and actionable is critical for Vulkan-related tasks.
2. **Cross-agent dependencies**  
   ✅ CONCUR — Misalignment in prompts can cause downstream agents to misinterpret tasks. Structured handoff formats and guild procedures are essential for maintaining consistency.
3. **Scope creep**  
   ⚠️ QUALIFY — While overcomplicating the prompt catalog may increase maintenance overhead, it is important to ensure that Vulkan-related prompts are detailed enough to prevent API misuse or synchronization issues.

## 8) Procedure Recording
### PROC-GPU-002 — Vulkan Prompt Refinement
1. Identify Vulkan-specific tasks in the prompt catalog.
2. Ensure prompts are precise, actionable, and aligned with Vulkan Guru's responsibilities.
3. Validate prompts with GPU Pipe

... [output truncated by factory gate] ...

## Kernel Expert
No code changes are required for this task, as it focuses on refining the prompt catalog.

## 7) Cross-Confirmation
### vs C++ Lead
**CONCUR**: The C++ Lead's findings regarding the need for clarity and consistency in the prompt catalog align with my observations. The proposed structure for the catalog is logical and supports modularity and maintainability.

### vs Vulkan Guru
**CONCUR**: The Vulkan Guru's emphasis on ensuring Vulkan-related prompts are precise and actionable is consistent with my focus on GPU-related prompts. The identified risks and constraints are valid and align with the shared mission and guardrails.

## 8) Procedure Recording
### PROC-PERF-003 — GPU-Specific Prompt Refinement
1. Identify GPU-related tasks in the prompt catalog.
2. Ensure prompts specify clear performance goals, constraints, and guidelines for shader optimization, workgroup sizing, and memory access patterns.
3. Add examples of common GPU-related tasks to improve clarity and usability.
4. Cross-confirm with Vulkan Guru and C++ Lead to ensure alignment with shared mission and execution mode.

## 9) Recommended Next Run
- **Macro**: FOLLOW
- **Task Command**: Refactorer to review and finalize the updated prompt catalog.

## Refactorer
```markdown
# Prompt Catalog

## Overview
This document defines the prompts used by agents in the GENERATIONS pipeline. Each prompt is designed to align with the shared mission, execution mode, and guardrails.

### Structure
- **Agent-Specific Prompts**: Prompts tailored to the unique responsibilities of each agent.
- **Shared Prompts**: Prompts that apply to all agents.
- **Guild-Specific Prompts**: Prompts that align with guild procedures and vocabulary.

## Agent-Specific Prompts

### C++ Lead
- Analyze and propose changes for the current task.
- Define and emit handoff TODOs for all other agents.

### Vulkan Guru
- Ensure Vulkan-related prompts are precise and actionable.
- Highlight potential synchronization and resource-lifecycle risks.

### Kernel Expert
- Ensure GPU-related prompts are precise and actionable.
- Highlight potential risks related to shader occupancy and workgroup sizing.

### Refactorer
- Ensure prompts are clear, modular, and maintainable.
- Align prompts with shared mission and execution mode.

### HPC Marketeer
- Translate technical prompts into user-friendly documentation.
- Ensure prompts are accessible and understandable to all agents.

## Shared Prompts
- Ensure all changes align with the shared mission and execution mode.
- Use structured handoff format and guild procedures to ensure consistency.

## Guild-Specific Prompts

### Architecture Guild
- Ensure prompts align with boundary rules and separation of concerns.
- Use PROC-ARCH-001 and PROC-ARCH-002 for renames and extractions.

### Performance Guild
- Ensure prompts align with performance optimization principles.
- Highlight potential performance bottlenecks and propose solutions.

### GPU Pipeline Guild
- Ensure prompts align with GPU pipeline optimization principles.
- Highlight potential risks related to GPU feature requirements.

## Notes
- Keep prompts specific, actionable, and aligned with the shared mission.
- Avoid overcomplicating prompts to reduce maintenance overhead.
```

## 7) Cross-Confirmation
### C++ Lead
- Confirmed that the proposed changes align with the shared mission and execution mode.
- No challenges to naming or boundary choices.

### Vulkan Guru
- Confirmed that Vulkan-related prompts are precise and actionable.
- No challenges to Vulkan-specific naming or boundary choices.

### Kernel Expert
- Confirmed that GPU-related prompts are precise and actionable.
- No challenges to shader-related naming or boundary choices.

## 8) Procedure Recording
### PROC-ARCH-003 — Prompt Restructuring
- **Origin**: Refactorer, 2023-10-10, FOLLOW-IN-THE-AGENTS-FOLDER-THE
- **Steps**:
  1. Review existing prompt catalog for clarity, modularity,

... [output truncated by factory gate] ...

## HPC Marketeer
```markdown
# Proposed Changes to `.github/agents/town/prompt-catalog.md`

## Example Prompt Refinement
### Before
"Ensure Vulkan resources are synchronized correctly."
### After
"Verify that Vulkan resources are synchronized using appropriate barriers and semaphores. Ensure that resource lifetimes are managed explicitly to avoid undefined behavior."

## Example Structural Improvement
### Before
- Task: "Optimize shaders for GPU performance."
- Task: "Ensure workgroup sizes are configured correctly."

### After
- **Shader Optimization Tasks**
  - "Optimize shaders for GPU performance by minimizing memory access latency."
  - "Profile and adjust workgroup sizes to maximize GPU occupancy."

# Proposed Changes to `README.md`

## Example Update
### Before
"Refer to the prompt catalog for agent-specific tasks."
### After
"Refer to the updated prompt catalog in `.github/agents/town/prompt-catalog.md` for detailed, role-specific tasks. The catalog has been refined to improve clarity and alignment with the GENERATIONS project goals."

# Proposed Changes to `assets/todo/`
- Add a new task: "Maintain alignment between the prompt catalog and project goals. Review and update the catalog quarterly."

# Proposed Changes to `assets/economic/`
- Ensure that references to agent tasks in the Austrian economics documentation are consistent with the updated prompt catalog.
```

## 7) Cross-Confirmation
| Agent         | Finding                                                                 | Status         |
|---------------|-------------------------------------------------------------------------|----------------|
| **C++ Lead**  | Prompt catalog requires updates for clarity and alignment.             | **CONCUR**     |
| **Vulkan Guru** | Vulkan-related prompts need to be precise and actionable.             | **CONCUR**     |
| **Kernel Expert** | GPU-related prompt

... [output truncated by factory gate] ...
