```markdown
# C++ Lead Agent Output

## 1) Main Task Outcome
The task involves refining the prompt catalog located in `.github/agents/town/prompt-catalog.md` to align with the GENERATIONS project goals and ensure the pipeline's successful execution. The updated catalog should reflect the shared mission, execution mode, guardrails, and guild-specific procedures outlined in the project documentation. The goal is to ensure that the prompts are clear, actionable, and aligned with the roles and responsibilities of each agent.

## 2) Secondary Task Outcomes
- Identified areas in the prompt catalog that require updates to align with the shared mission and execution mode.
- Highlighted opportunities to improve clarity and consistency in the prompts to reduce ambiguity for downstream agents.
- Proposed a structure for the prompt catalog to ensure it is modular, maintainable, and easy to extend.

## 3) Risks and Constraints
| Risk/Constraint | Description | Mitigation |
|------------------|-------------|------------|
| **Ambiguity in prompts** | Vague or unclear prompts may lead to inconsistent agent behavior. | Ensure all prompts are specific, actionable, and aligned with the shared mission. |
| **Cross-agent dependencies** | Misalignment in prompts may cause downstream agents to misinterpret tasks. | Use structured handoff format and guild procedures to ensure consistency. |
| **Scope creep** | Overcomplicating the prompt catalog may increase maintenance overhead. | Follow the shared guardrails to avoid unnecessary complexity. |
| **Documentation drift** | If the prompt catalog is not updated alongside other documentation, it may become outdated. | Synchronize updates with the shared review checklist. |

## 4) Actionable TODOs
### Vulkan Guru TODOs
- [ ] `.github/agents/town/prompt-catalog.md` — Review Vulkan-specific prompts for alignment with Vulkan resource semantics and synchronization [RISK: low].
- [ ] GUILD:gpu-pipeline — Validate that Vulkan-related prompts align with GPU Pipeline Guild procedures.

### Kernel Expert TODOs
- [ ] `.github/agents/town/prompt-catalog.md` — Review prompts related to shader development and GPU utilization for accuracy and completeness [RISK: low].
- [ ] GUILD:performance — Ensure prompts related to GPU performance follow PROC-PERF-001 and PROC-PERF-002.

### Refactorer TODOs
- [ ] `.github/agents/town/prompt-catalog.md` — Review and refine prompt naming and structure for clarity and consistency [RISK: low].
- [ ] GUILD:architecture — Validate that prompt updates align with architectural boundaries and naming conventions.

### HPC Marketeer TODOs
- [ ] `.github/agents/town/prompt-catalog.md` — Update documentation to reflect changes in the prompt catalog and ensure alignment with user-facing materials [RISK: low].
- [ ] `README.md` — Add a section explaining the role of the prompt catalog in the GENERATIONS pipeline.

## 5) Handoff Note
The prompt catalog has been analyzed, and a proposed structure for refinement has been outlined. Downstream agents should review and refine the prompts within their areas of expertise to ensure alignment with the shared mission, execution mode, and guild procedures. Specific TODOs have been provided for each agent to address their respective responsibilities.

## 6) Code Proposal
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