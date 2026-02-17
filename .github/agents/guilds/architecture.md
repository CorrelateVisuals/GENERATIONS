# Architecture Guild

## Members
- **C++ Lead** — ownership boundaries, include dependencies, lifecycle correctness
- **Refactorer** — naming, separation of concerns, dead code, complexity reduction

## Shared Vocabulary
| Term | Meaning |
|---|---|
| **boundary** | Compile-unit separation between src/ subdirectories (engine, vulkan_base, vulkan_mechanics, vulkan_pipelines, vulkan_resources, world, control, library) |
| **leak** | A dependency that crosses a boundary in the wrong direction |
| **owner** | The single class/module responsible for a resource's lifetime |
| **seam** | An interface point where two modules interact — smallest viable API surface |
| **migration** | Multi-step refactor with each step compiling and running |

## Decision Protocol
When members disagree on structure:
1. Does it compile and run after each step? (non-negotiable)
2. Does it reduce the include graph? Prefer yes
3. Does it remove code? Prefer yes over moving code
4. Does it add abstraction? Only if it removes duplication elsewhere
5. Tie-breaker: C++ Lead owns the final call on API surface, Refactorer owns naming

## Boundary Rules
```
engine/ → can depend on everything below
world/ → can depend on vulkan_pipelines/, vulkan_resources/, library/
vulkan_pipelines/ → can depend on vulkan_base/, vulkan_mechanics/, vulkan_resources/
vulkan_mechanics/ → can depend on vulkan_base/
vulkan_resources/ → can depend on vulkan_base/
vulkan_base/ → depends only on Vulkan SDK + libraries/
control/ → can depend on world/, engine/
library/ → depends only on STL + libraries/
```
Validate with `assets/tools/check_folder_dependencies.py`.

## Common Procedures
<!-- Agents append discovered patterns here using ## PROC-ARCH-NNN format -->

### PROC-ARCH-001 — Safe Rename Sequence
1. Refactorer proposes new name + rationale
2. C++ Lead verifies no ABI/API break (especially if name appears in shader interface)
3. Apply rename in one commit, no behavior change
4. Update docs in a separate follow-up commit

### PROC-ARCH-002 — Extract-and-Isolate
1. Identify the code block to extract (C++ Lead or Refactorer)
2. Create the new file/function with a clear single responsibility
3. Add include/forward-declaration at the boundary seam
4. Verify dependency direction with check_folder_dependencies.py
5. Remove the original inline code
