# GENERATIONS

![GENERATIONS Cover](assets/TerrainCover_20260215.png)

<p align="left">
	<img src="assets/branding/vulkan.svg" alt="Vulkan" height="104" />
	&nbsp;&nbsp;
	<img src="assets/branding/cplusplus.svg" alt="C++" height="52" />
	&nbsp;&nbsp;
	<img src="assets/branding/cmake.svg" alt="CMake" height="52" />
</p>

GENERATIONS is a Vulkan-native simulation runtime built for reliable, high-throughput GPU workloads.
It is powered by CAPITAL Engine and designed around one idea: keep the stack lean, explicit, and fast enough to support serious compute-driven interactive systems.

Today, the project runs a real-time cellular simulation with the latest terrain pass, rendered through a modern graphics + compute pipeline with strong separation between platform, rendering, and world logic.

![Runtime Snapshot](assets/TerrainRuntimeTopDown_20260215.png)

## Why this project exists

This is software for building real things:

- GPU-first simulation pipelines
- repeatable rendering behavior
- robust architecture boundaries that hold up under growth

## What is running now

The current runtime demonstrates Conway-style cellular evolution with GPU compute, real-time rendering, terrain, and post-processing.
The system is tested in active Linux development and includes a maintained Visual Studio solution for Windows workflows.

## Architecture (current state)

The source tree is intentionally layered:

```text
src/
├── app/        # application entry and lifecycle orchestration
├── base/       # Vulkan primitives, memory/resources, sync, descriptors, pipelines
├── core/       # logging and timing
├── io/         # utility I/O (assets, screenshots, helpers)
├── platform/   # window/input/validation integration
├── render/     # render mechanics, command recording, pipeline wiring
└── world/      # simulation state, camera, geometry, terrain
```

Dependency boundaries are checked through:

- `tools/check_folder_dependencies.py`

## Core runtime flow

At runtime, the engine executes a stable loop:

1. update world and uniform state
2. dispatch compute work
3. record and submit graphics work
4. present and handle resize/swapchain lifecycle

This separation is what makes the project suitable for future node-based and TouchDesigner-style workflows.

## C++ implementation interface

The implementation-layer graph setup is now pure C++.

- Engine mode (default): `./bin/CapitalEngine`
- Implementation preview only (no engine loop): `CE_SCRIPT_ONLY=1 ./bin/CapitalEngine`

Runtime execution plan, pipeline definitions, draw bindings, and world/terrain settings are installed from `src/implementation/ScriptChainerApp.cpp`.
Engine runtime stays in `src/app`, while implementation configuration lives in `src/implementation`.

## Environment flags

Runtime env flags are parsed centrally through `CE::Runtime::env_truthy` in `src/core/RuntimeConfig.*`.

- accepted truthy values (case-insensitive): `1`, `true`, `on`
- everything else (including unset) is treated as `false`

Current flags used by the runtime:

- `CE_SCRIPT_ONLY`: run implementation setup only, skip main engine loop when truthy
- `CE_STARTUP_SCREENSHOT`: capture one startup screenshot when truthy
- `CE_GPU_TRACE`: enable detailed GPU trace logging when truthy
- `CE_CAMERA_TUNING`: enable live camera tuning controls (`T` toggle, `,`/`.` select, `[`/`]` adjust)

GPU log tuning flags:

- `CE_GPU_LOG`: master switch for GPU logs (`on`/`off`), default `on`
- `CE_GPU_LOG_STARTUP`: startup GPU snapshot logs (`on`/`off`), default `on`
- `CE_GPU_LOG_PERIODIC`: periodic runtime GPU samples in main loop (`on`/`off`), default `off`
- `CE_GPU_LOG_FREQ_MS`: periodic sample interval in milliseconds (minimum `250`), default `5000`
- `CE_GPU_LOG_DETAILS`: GPU log detail mode (`basic` or `detailed`), default `basic`

Compute workload presets:

- `CE_WORKLOAD_PRESET`: `default` (graphics + compute), `compute_only` (pre-compute only)
- `CE_COMPUTE_CHAIN`: comma-separated list of compute pipeline names to run in order
	(defaults to `ComputeInPlace,ComputeJitter,ComputeCopy` when `CE_WORKLOAD_PRESET` is compute-only)

Example (compute-only chain):

```bash
CE_WORKLOAD_PRESET=compute_only ./bin/CapitalEngine
```

Console/terminal behavior:

- `NO_COLOR` disables ANSI log colors

## Build and run (Linux)

### Requirements (Linux)

- CMake >= 3.20
- GCC 12 / G++ 12 (configured in CMake)
- Vulkan SDK 1.3.x (Vulkan loader + headers)
- GLFW 3.3.x (linkable as `glfw`)
- Python 3 (for dependency boundary checks)
- `glslangValidator` in PATH (shader compilation)

If you need a different compiler, override in your configure step:

```bash
cmake -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

From project root:

```bash
cmake -S . -B build -Wno-dev --log-level=NOTICE
cmake --build build --parallel $(nproc)
./bin/CapitalEngine
```

Notes:

- output binary: `bin/CapitalEngine`
- shaders are compiled at runtime on first use (generates `shaders/*.spv`)
- `glslangValidator` must be available in your PATH
- compute-only presets use `shaders/ComputeInPlace.comp`, `ComputeJitter.comp`, and `ComputeCopy.comp`

## Build and run (Windows)

Open `GENERATIONS.sln` in Visual Studio 2022, ensure Vulkan SDK and GLFW are configured, then build `CapitalEngine`.

### Requirements (Windows)

- Visual Studio 2022 (v143 toolset)
- Vulkan SDK 1.3.x
- GLFW 3.3.x
- GLM (headers only)
- `glslangValidator` in PATH (shader compilation)

The Visual Studio project expects Vulkan SDK headers and libs installed in the default SDK location, and GLFW/GLM available in a shared `Libraries` folder (see the include/lib directories in the project file).

The committed project files in `src/` (`CAPITAL-engine.vcxproj*`) are kept in sync with the CMake-based Linux flow.

## Development standards

Formatting:

```bash
find src -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
```

Architecture boundary check:

```bash
python3 tools/check_folder_dependencies.py
```

## Design direction

GENERATIONS is being shaped into a dependable foundation for high-performance, reusable simulation software.
The near-term direction is stronger resource reuse, cleaner operator abstractions, and predictable graph-style execution without sacrificing Vulkan-level control.

In short: powerful enough for HPC-minded workloads, straightforward enough to iterate quickly.

---

## Adding a second terrain grid (engine internals guide)

This section documents every insertion point required to place a second, independent terrain grid next to the existing one. The second grid gets its own geometry, cells, compute, and can have a different seed, different terrain noise, or different dimensions.

### Recommended approach: UBO-per-draw update

The simplest path is to keep the UBO struct unchanged and update `ubo.model` (translation offset) and `ubo.grid_xy` between draw calls. This means **no shader changes** are needed — only C++ modifications.

### Subsystem-by-subsystem insertion points

#### 1. World (owns the grid)

**[src/world/World.h](src/world/World.h)**

The `World` class owns a single `Grid _grid` member. Add a second member:

```cpp
Grid _grid;       // existing
Grid _grid2;      // ← add: second terrain instance
```

The `Grid` struct inherits from `Geometry` and is fully self-contained (owns its own vertex buffer, index buffer, box buffers, cells array, coordinates). A second instance gets its own GPU buffers automatically through construction.

**[src/world/World.cpp](src/world/World.cpp)**

| Location | What to change |
|----------|---------------|
| Constructor initializer list (L33) | Add `_grid2(terrain_settings_2, command_buffer, command_pool, queue)` with a different `TerrainSettings` or the same with a position offset |
| Grid origin (`startX`, `startY` — L108) | Each grid needs its own base offset. Currently centered at origin. For side-by-side: `_grid2` could use `startX += grid1_width` |
| Cell `instance_position` (L119–128) | The per-grid base offset applies here too |
| Camera `configure_arcball` (L73–78) | Recompute `sceneRadius` from the **combined** bounding box of both grids |

No changes needed to [src/world/Geometry.h](src/world/Geometry.h), [src/world/Geometry.cpp](src/world/Geometry.cpp), [src/world/Terrain.h](src/world/Terrain.h), or [src/world/Terrain.cpp](src/world/Terrain.cpp) — they are fully reusable per-instance.

#### 2. UBO and shader interface

**[src/core/ShaderInterface.h](src/core/ShaderInterface.h)**

The current UBO carries a single `grid_xy` and a single `model` matrix (currently identity). With the UBO-per-draw approach, these fields are **reused** — just memcpy different values before each grid draw. No struct changes needed.

If you later want both grids visible in a single shader invocation (e.g., for inter-grid interactions in compute), you would add `ivec2 grid2_xy` and `mat4 model2` to the UBO and mirror in [shaders/ParameterUBO.glsl](shaders/ParameterUBO.glsl).

#### 3. Resources (GPU buffers and descriptors)

**[src/render/Resources.h](src/render/Resources.h)**

| Member | What to change |
|--------|---------------|
| `World world` | Stays single — carries both grids internally |
| `StorageBuffer shader_storage` | This is the SSBO pair (buffer_in / buffer_out) sized to `_grid.point_count`. Add a second `StorageBuffer shader_storage_2` for grid2's cells |

**[src/render/Resources.cpp](src/render/Resources.cpp)**

| Location | What to change |
|----------|---------------|
| L34–35 | `shader_storage{..., world._grid.cells, world._grid.point_count}` — duplicate for `shader_storage_2{..., world._grid2.cells, world._grid2.point_count}` |
| L122–127 | `update()` sets `ubo.grid_xy` and `ubo.model` from a single grid. For two grids, update these before each draw call sequence |

**[src/base/VulkanCore.h](src/base/VulkanCore.h)**

```cpp
constexpr size_t NUM_DESCRIPTORS = 5;  // ← increase to 7 for 2 extra SSBO bindings
```

The descriptor system ([VulkanDescriptor.h](src/base/VulkanDescriptor.h) / [VulkanDescriptor.cpp](src/base/VulkanDescriptor.cpp)) uses `NUM_DESCRIPTORS`-sized arrays and adapts automatically — only the constant needs changing.

#### 4. Shader access (draw command recording)

**[src/render/ShaderAccess.cpp](src/render/ShaderAccess.cpp)** — the most affected file.

Graphics draw lambdas:

| Lambda | Grid reference | What to change |
|--------|---------------|----------------|
| `draw_cells` (L195) | `_grid.size`, `shader_storage.buffer_out/in` | Duplicate for grid2: use `_grid2.size`, bind `shader_storage_2` |
| `draw_grid_indexed` (L203) | `_grid.vertex_buffer`, `_grid.index_buffer` | Duplicate: use `_grid2.vertex_buffer`, `_grid2.index_buffer` |
| `draw_grid_box_indexed` (L212) | `_grid.box_vertex_buffer`, `_grid.box_index_buffer` | Duplicate: use `_grid2.box_*` buffers |

Between draw calls for grid1 and grid2: memcpy the updated UBO (with offset model matrix and grid2 dimensions) before recording grid2's draw commands.

Compute dispatch (L32–99):

Currently dispatches with work groups sized for the single grid and a single SSBO binding. For grid2's cells, issue a second dispatch with re-bound SSBO (bindings 5–6) and updated `gridXY`.

#### 5. Pipeline configuration

**[src/render/Pipelines.h](src/render/Pipelines.h)** and **[src/render/Pipelines.cpp](src/render/Pipelines.cpp)**

Work groups are computed from a single `grid_size` (L50–68, L96–104). For two grids with different dimensions, either:
- Compute work groups as `max(grid1, grid2)` if they share compute pipelines
- Use per-grid work group counts for separate dispatches

**[src/render/Mechanics.cpp](src/render/Mechanics.cpp)** (L26) — `refresh_dynamic_work_groups` also takes a single `grid_size`. Pass the combined or max dimensions.

#### 6. Implementation spec and render graph

**[src/implementation/ImplementationSpec.h](src/implementation/ImplementationSpec.h)**

```cpp
CE::Runtime::TerrainSettings terrain{};   // existing
CE::Runtime::TerrainSettings terrain2{};  // ← add
```

**[src/implementation/ImplementationSpec.cpp](src/implementation/ImplementationSpec.cpp)**

| Location | What to change |
|----------|---------------|
| L89–113 | Populate `spec.terrain2` with dimensions, cell size, noise seed, etc. |
| L156–170 | Add pipeline definitions if using separate pipeline names (e.g., `"Landscape2"`) — or reuse existing names and draw twice |
| L212–221 | `spec.draw_ops` — add `{"Landscape2", "indexed:grid2"}` |
| L222–235 | `spec.render_graph` — add `"Landscape2"`, `"TerrainBox2"` entries in the graphics nodes |

**[src/core/RuntimeConfig.h](src/core/RuntimeConfig.h)** — add `DrawOpId::IndexedGrid2`, `IndexedGrid2Box`.

**[src/core/RuntimeConfig.cpp](src/core/RuntimeConfig.cpp)** — add parsing for `"indexed:grid2"` in `draw_op_from_string()`.

#### 7. Application entry

**[src/app/CapitalEngine.cpp](src/app/CapitalEngine.cpp)** (L15–16) — currently fetches one `TerrainSettings` and passes it to `Resources`. Pass both terrain configs.

#### 8. Shaders (no changes with UBO-per-draw)

Every terrain shader reads `ubo.gridXY` and `ubo.model`. With the per-draw update approach, these values are correct for whichever grid is being drawn. **No shader source changes needed.**

If switching to inter-grid compute (cells from grid1 affecting grid2), the compute shaders ([Engine.comp](shaders/Engine.comp), [ComputeInPlace.comp](shaders/ComputeInPlace.comp), etc.) would need additional SSBO bindings for the second grid's cell buffer.

### Files checklist

| File | Change required | Reason |
|------|:-:|--------|
| [src/world/World.h](src/world/World.h) | **Yes** | Add `Grid _grid2` member |
| [src/world/World.cpp](src/world/World.cpp) | **Yes** | Construct `_grid2`, adjust camera bounding |
| [src/world/Geometry.h](src/world/Geometry.h) | No | Reusable per-instance |
| [src/world/Geometry.cpp](src/world/Geometry.cpp) | No | Reusable per-instance |
| [src/world/Terrain.h](src/world/Terrain.h) | No | Stateless generator |
| [src/world/Terrain.cpp](src/world/Terrain.cpp) | No | Stateless generator |
| [src/core/ShaderInterface.h](src/core/ShaderInterface.h) | Maybe | Only if adding per-grid UBO fields |
| [src/core/RuntimeConfig.h](src/core/RuntimeConfig.h) | **Yes** | Add `DrawOpId::IndexedGrid2` |
| [src/core/RuntimeConfig.cpp](src/core/RuntimeConfig.cpp) | **Yes** | Parse new draw op strings |
| [src/render/Resources.h](src/render/Resources.h) | **Yes** | Add second `StorageBuffer` |
| [src/render/Resources.cpp](src/render/Resources.cpp) | **Yes** | Init second SSBO, per-grid UBO update |
| [src/render/ShaderAccess.cpp](src/render/ShaderAccess.cpp) | **Yes** | Duplicate draw lambdas, bind grid2 buffers |
| [src/render/Pipelines.h](src/render/Pipelines.h) | **Yes** | Handle two grid sizes for work groups |
| [src/render/Pipelines.cpp](src/render/Pipelines.cpp) | **Yes** | Pass both grid sizes |
| [src/render/Mechanics.cpp](src/render/Mechanics.cpp) | **Yes** | Both sizes on swapchain recreate |
| [src/render/FrameContext.h](src/render/FrameContext.h) | No | No direct grid references |
| [src/render/FrameContext.cpp](src/render/FrameContext.cpp) | No | No direct grid references |
| [src/base/VulkanCore.h](src/base/VulkanCore.h) | **Yes** | Increase `NUM_DESCRIPTORS` (5 → 7) |
| [src/implementation/ImplementationSpec.h](src/implementation/ImplementationSpec.h) | **Yes** | Add `TerrainSettings terrain2` |
| [src/implementation/ImplementationSpec.cpp](src/implementation/ImplementationSpec.cpp) | **Yes** | Populate terrain2, render graph entries |
| [src/app/CapitalEngine.cpp](src/app/CapitalEngine.cpp) | **Yes** | Pass both terrain configs |
| [shaders/ParameterUBO.glsl](shaders/ParameterUBO.glsl) | Maybe | Only if UBO struct changes |
| All `.vert` / `.frag` / `.comp` shaders | No* | UBO-per-draw update keeps them valid |

\* No changes needed with the per-draw UBO update approach. Would need changes only if the UBO struct is modified or inter-grid compute is added.

### Minimum viable insertion order

1. **ImplementationSpec** — add `terrain2` settings, pipeline defs, draw ops, render graph entries
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


