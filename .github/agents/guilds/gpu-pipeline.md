# GPU Pipeline Guild

## Members
- **Vulkan Guru** — API-level resource management, synchronization, descriptor binding
- **Kernel Expert** — shader-level data consumption, dispatch geometry, workgroup optimization

## Shared Vocabulary
| Term | Meaning |
|---|---|
| **interface** | The C++/GLSL boundary: push constants, UBO layout, SSBO binding, vertex attributes |
| **layout** | Memory arrangement: std140/std430, binding index, set index |
| **dispatch** | Compute shader invocation: workgroup count × workgroup size = total invocations |
| **barrier** | Synchronization point: pipeline barrier, memory barrier, execution dependency |
| **stage** | Pipeline stage: vertex → tesc → tese → geometry → fragment, or compute |

## Decision Protocol
When members disagree on GPU work:
1. Interface changes require BOTH members to agree (shader + API must match)
2. Layout changes: Kernel Expert proposes optimal layout, Vulkan Guru validates API-side alignment
3. Barrier placement: Vulkan Guru owns correctness, Kernel Expert flags if it kills occupancy
4. New pipeline: Vulkan Guru creates the pipeline object, Kernel Expert writes the shader
5. Tie-breaker: correctness (Vulkan Guru) beats performance (Kernel Expert)

## Interface Contract
Changes to these files require both guild members to review:
- `shaders/ParameterUBO.glsl` ↔ `src/vulkan_pipelines/ShaderInterface.h`
- `shaders/Engine.comp` ↔ `src/vulkan_pipelines/ShaderAccess.cpp` (dispatch dimensions)
- Any `layout(binding=N)` change in shaders ↔ descriptor set layout in `VulkanBaseDescriptor.cpp`

## Common Procedures
<!-- Agents append discovered patterns here using ## PROC-GPU-NNN format -->

### PROC-GPU-001 — Shader Interface Change
1. Kernel Expert proposes the GLSL-side change (new uniform, SSBO field, push constant)
2. Vulkan Guru implements the C++ side (descriptor set, push constant range, buffer update)
3. Both verify layout alignment (std140 padding rules, binding indices)
4. Run with validation layers enabled to catch mismatches

### PROC-GPU-002 — Compute Dispatch Sizing
1. Kernel Expert determines optimal workgroup size for the problem domain
2. Vulkan Guru calculates dispatch dimensions: ceil(totalInvocations / workgroupSize)
3. Kernel Expert adds bounds check in shader for edge-case invocations
4. Both verify specialization constants if workgroup size is configurable
