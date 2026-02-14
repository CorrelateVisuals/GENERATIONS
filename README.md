# GENERATIONS

![GENERATIONS Cover](assets/CoverCapture.PNG)

<p align="left">
	<img src="assets/branding/vulkan.svg" alt="Vulkan" height="52" />
	&nbsp;&nbsp;
	<img src="assets/branding/cplusplus.svg" alt="C++" height="52" />
	&nbsp;&nbsp;
	<img src="assets/branding/cmake.svg" alt="CMake" height="52" />
</p>

GENERATIONS is a Vulkan-native simulation runtime built for reliable, high-throughput GPU workloads.
It is powered by CAPITAL Engine and designed around one idea: keep the stack lean, explicit, and fast enough to support serious compute-driven interactive systems.

Today, the project runs a real-time cellular simulation and renders it through a modern graphics + compute pipeline with strong separation between platform, rendering, and world logic.

![Runtime Snapshot](assets/LandscapeCapture.PNG)

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

Dependency boundaries are documented and checked through:

- `src/FOLDER_DEPENDENCY_MAP.md`
- `tools/check_folder_dependencies.py`

This keeps feature velocity high as the codebase scales.

## Core runtime flow

At runtime, the engine executes a stable loop:

1. update world and uniform state
2. dispatch compute work
3. record and submit graphics work
4. present and handle resize/swapchain lifecycle

This separation is what makes the project suitable for future node-based and TouchDesigner-style workflows.

## Build and run (Linux)

From project root:

```bash
cmake -S . -B build -Wno-dev --log-level=NOTICE
cmake --build build --parallel $(nproc)
./bin/CapitalEngine
```

Notes:

- output binary: `bin/CapitalEngine`
- shader binaries (`.spv`) are generated in `shaders/`
- `glslangValidator` or `glslc` must be available in your PATH

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

Built on lessons from the Vulkan ecosystem, including the excellent learning material at https://vulkan-tutorial.com/.

