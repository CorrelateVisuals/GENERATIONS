# Current Manual Task

## Task ID
CE-AUDIT-001

## Manual Action Statement
Audit the GENERATIONS Vulkan engine for code quality, performance, and correctness across the rendering pipeline and compute simulation.

## Scope
- In scope:
  - `src/vulkan_base/*`
  - `src/vulkan_mechanics/*`
  - `src/vulkan_pipelines/*`
  - `src/vulkan_resources/*`
  - `src/engine/*`
  - `src/world/*`
  - `shaders/Engine.comp`
  - `shaders/PostFX.comp`
  - `shaders/ParameterUBO.glsl`
- Out of scope:
  - Third-party libraries (`libraries/`)
  - Asset files (`assets/3D/`)
  - Build system internals

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
