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
- `CE_GPU_TRACE`: enable detailed GPU trace logging and profiling when truthy

GPU log tuning flags:

- `CE_GPU_LOG`: master switch for GPU logs (`on`/`off`), default `on`
- `CE_GPU_LOG_STARTUP`: startup GPU snapshot logs (`on`/`off`), default `on`
- `CE_GPU_LOG_PERIODIC`: periodic runtime GPU samples in main loop (`on`/`off`), default `off`
- `CE_GPU_LOG_FREQ_MS`: periodic sample interval in milliseconds (minimum `250`), default `5000`
- `CE_GPU_LOG_DETAILS`: GPU log detail mode (`basic` or `detailed`), default `basic`

Console/terminal behavior:

- `NO_COLOR` disables ANSI log colors

## GPU Profiler

The GPU profiler provides lightweight monitoring of GPU-CPU communication, buffer transfers, and synchronization waits.

**Features:**
- GPU timestamp queries for precise GPU operation timing
- CPU-side timing for synchronization waits
- Automatic tracking of buffer copies and transfers
- Periodic performance reports

**Usage:**

Enable GPU profiling by setting the `CE_GPU_TRACE` environment variable:

```bash
CE_GPU_TRACE=1 ./bin/CapitalEngine
```

When enabled, the profiler automatically tracks:
- Buffer copy operations (GPU_Buffer_Copy, Buffer::copy)
- Buffer-to-image transfers (GPU_Buffer_To_Image, Buffer::copy_to_image)
- Fence wait times (VkFence_Wait)

Performance reports are printed every 300 frames (~5 seconds at 60fps) showing:
- Individual event timings in milliseconds
- Event type (GPU or CPU)
- Total GPU and CPU time per reporting period

**Example Output:**
```
┌─────────────────────────────────────────────────────────┐
│           GPU Profiler Report                           │
├─────────────────────────────────────────────────────────┤
│ GPU_Buffer_Copy                              0.023 ms [GPU]
│ Buffer::copy                                 0.156 ms [CPU]
│ VkFence_Wait                                 0.012 ms [CPU]
│ GPU_Buffer_To_Image                          0.045 ms [GPU]
│ Buffer::copy_to_image                        0.198 ms [CPU]
├─────────────────────────────────────────────────────────┤
│ Total GPU time:      0.068 ms
│ Total CPU time:      0.366 ms
└─────────────────────────────────────────────────────────┘
```

## Build and run (Linux)

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

## Build and run (Windows)

Open `GENERATIONS.sln` in Visual Studio 2022, ensure Vulkan SDK and GLFW are configured, then build `CapitalEngine`.

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


