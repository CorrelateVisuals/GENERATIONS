# GENERATIONS

![GENERATIONS Cover](assets/TerrainCover_20260215.png)

<p align="left">
	<img src="assets/branding/vulkan.svg" alt="Vulkan" height="104" />
	&nbsp;&nbsp;
	<img src="assets/branding/cplusplus.svg" alt="C++" height="52" />
	&nbsp;&nbsp;
	<img src="assets/branding/cmake.svg" alt="CMake" height="52" />
</p>

GENERATIONS is a Vulkan-native simulation runtime (CAPITAL Engine) focused on GPU-first simulation + rendering.
The project currently runs a Conway-style cellular simulation on terrain with compute/graphics staging and runtime-configurable pipeline execution.

![Runtime Snapshot](assets/TerrainRuntimeTopDown_20260215.png)

## Current status (February 2026)

- Linux build is green with CMake (`cmake -S . -B build && cmake --build build -j4`)
- Runtime launch is verified from the generated binary (`./bin/CapitalEngine`)
- Scene setup is authored in C++ via `src/world/SceneConfig.*`
- Runtime config registry/env parsing lives in `src/world/RuntimeConfig.*`
- Resource/world runtime objects are in `src/resources/*`
- Vulkan pipeline + frame orchestration are in `src/vulkan_pipelines/*`

## Source layout (current)

```text
src/
├── main.cpp
├── control/           # window, timer, GUI/stage-strip controls
├── engine/            # CAPITAL Engine lifecycle + logging
├── library/           # path helpers, screenshots, utility I/O
├── resources/         # World + Resources + generated shader interface
├── vulkan_base/       # low-level Vulkan wrappers and primitives
├── vulkan_mechanics/  # device/swapchain orchestration
├── vulkan_pipelines/  # pipeline config + frame command recording
└── world/             # scene defaults, runtime config, camera, geometry, terrain
```

## Build and run (Linux)

### Requirements

- CMake >= 3.20
- GCC 12 / G++ 12 (default in `CMakeLists.txt`)
- Vulkan SDK 1.3.x
- GLFW 3.3.x (`glfw`)
- Python 3
- `glslangValidator` in `PATH`

### Commands

```bash
cmake -S . -B build -Wno-dev --log-level=NOTICE
cmake --build build --parallel $(nproc)
./bin/CapitalEngine
```

## Runtime modes and flags

- `CE_SCRIPT_ONLY=1`: apply scene config only, skip engine loop
- `CE_WORKLOAD_PRESET=default|compute_only`: select workload strategy
- `CE_COMPUTE_CHAIN=<csv>`: set compute pipeline order
- `CE_RENDER_STAGE=<n>`: restrict stage selection via scene config
- `CE_STARTUP_SCREENSHOT=1`: capture startup screenshot
- `CE_GPU_TRACE=1`: verbose GPU trace logging
- `CE_CAMERA_TUNING=1`: enable camera tuning controls
- `NO_COLOR=1`: disable ANSI-colored logs

Truthy env values are centrally parsed by `CE::Runtime::env_truthy` and accept `1`, `true`, `on` (case-insensitive).

## Build and run (Windows)

Use `GENERATIONS.sln` (Visual Studio 2022, v143 toolset) with Vulkan SDK + GLFW configured.

## Developer checks

```bash
python3 tools/check_folder_dependencies.py
```

```bash
find src -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
```

**[src/world/RuntimeConfig.cpp](src/world/RuntimeConfig.cpp)** — add parsing for `"indexed:grid2"` in `draw_op_from_string()`.

#### 7. Application entry

**[src/engine/CapitalEngine.cpp](src/engine/CapitalEngine.cpp)** (L15–16) — currently fetches one `TerrainSettings` and passes it to `Resources`. Pass both terrain configs.

#### 8. Shaders (no changes with UBO-per-draw)

Every terrain shader reads `ubo.gridXY` and `ubo.model`. With the per-draw update approach, these values are correct for whichever grid is being drawn. **No shader source changes needed.**

If switching to inter-grid compute (cells from grid1 affecting grid2), the compute shaders ([Engine.comp](shaders/Engine.comp), [ComputeInPlace.comp](shaders/ComputeInPlace.comp), etc.) would need additional SSBO bindings for the second grid's cell buffer.

### Files checklist

| File | Change required | Reason |
|------|:-:|--------|
| [src/World.h](src/World.h) | **Yes** | Add `Grid _grid2` member |
| [src/World.cpp](src/World.cpp) | **Yes** | Construct `_grid2`, adjust camera bounding |
| [src/world/Geometry.h](src/world/Geometry.h) | No | Reusable per-instance |
| [src/world/Geometry.cpp](src/world/Geometry.cpp) | No | Reusable per-instance |
| [src/world/Terrain.h](src/world/Terrain.h) | No | Stateless generator |
| [src/world/Terrain.cpp](src/world/Terrain.cpp) | No | Stateless generator |
| [src/resources/ShaderInterface.h](src/resources/ShaderInterface.h) | Maybe | Only if adding per-grid UBO fields |
| [src/world/RuntimeConfig.h](src/world/RuntimeConfig.h) | **Yes** | Add `DrawOpId::IndexedGrid2` |
| [src/world/RuntimeConfig.cpp](src/world/RuntimeConfig.cpp) | **Yes** | Parse new draw op strings |
| [src/Resources.h](src/Resources.h) | **Yes** | Add second `StorageBuffer` |
| [src/Resources.cpp](src/Resources.cpp) | **Yes** | Init second SSBO, per-grid UBO update |
| [src/pipelines/ShaderAccess.cpp](src/pipelines/ShaderAccess.cpp) | **Yes** | Duplicate draw lambdas, bind grid2 buffers |
| [src/Pipelines.h](src/Pipelines.h) | **Yes** | Handle two grid sizes for work groups |
| [src/Pipelines.cpp](src/Pipelines.cpp) | **Yes** | Pass both grid sizes |
| [src/vulkan_mechanics/Mechanics.cpp](src/vulkan_mechanics/Mechanics.cpp) | **Yes** | Both sizes on swapchain recreate |
| [src/pipelines/FrameContext.h](src/pipelines/FrameContext.h) | No | No direct grid references |
| [src/pipelines/FrameContext.cpp](src/pipelines/FrameContext.cpp) | No | No direct grid references |
| [src/vulkan_base/VulkanCore.h](src/vulkan_base/VulkanCore.h) | **Yes** | Increase `NUM_DESCRIPTORS` (5 → 7) |
| [src/world/SceneConfig.h](src/world/SceneConfig.h) | **Yes** | Add `TerrainSettings terrain2` |
| [src/world/SceneConfig.cpp](src/world/SceneConfig.cpp) | **Yes** | Populate terrain2, render graph entries |
| [src/engine/CapitalEngine.cpp](src/engine/CapitalEngine.cpp) | **Yes** | Pass both terrain configs |
| [shaders/ParameterUBO.glsl](shaders/ParameterUBO.glsl) | Maybe | Only if UBO struct changes |
| All `.vert` / `.frag` / `.comp` shaders | No* | UBO-per-draw update keeps them valid |

\* No changes needed with the per-draw UBO update approach. Would need changes only if the UBO struct is modified or inter-grid compute is added.

### Minimum viable insertion order

1. **SceneConfig** — add `terrain2` settings, pipeline defs, draw ops, render graph entries
2. **RuntimeConfig** — add `DrawOpId::IndexedGrid2` + parsing
3. **World** — add `_grid2` member + construct with offset origin
4. **VulkanCore** — bump `NUM_DESCRIPTORS`
5. **Resources** — add second `StorageBuffer`, second SSBO init
6. **ShaderAccess** — add draw lambdas for grid2, UBO memcpy between draws
7. **Pipelines / Mechanics** — pass combined grid sizes
8. **CapitalEngine** — pass both terrain settings through

## Thanks

Big thanks to everyone contributing through GitHub issues, reviews, and code:

- Jakob Povel
- Johannes Unterguggenberger (@johannes.unterguggenberger)
- Jos Onokiewicz (@jos.onokiewicz)

And gratitude to the open graphics ecosystem this project learns from and builds on, especially:

- https://vulkan-tutorial.com/
- https://github.com/SaschaWillems/Vulkan
- Khronos Vulkan documentation and tooling

---


