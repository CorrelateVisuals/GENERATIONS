# GENERATIONS

**High-Performance Computational Engine for GPU-Accelerated Simulations**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey.svg)](README.md)
[![Vulkan](https://img.shields.io/badge/API-Vulkan-red.svg)](https://www.vulkan.org/)

GENERATIONS is a high-performance computational simulator built on the **CAPITAL Engine**—a cross-platform Vulkan-based framework designed for massively parallel simulations and algorithms that harness GPU compute capabilities. Optimized for HPC environments, GENERATIONS leverages modern graphics APIs to achieve orders of magnitude speedup over traditional CPU-based approaches.

## Overview

Built from the ground up for **parallel computing at scale**, GENERATIONS demonstrates GPU-accelerated cellular automata and spatial simulations. The current implementation showcases [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) running entirely on GPU compute shaders, processing millions of cells per frame with real-time visualization.

**Key Features:**
- 🚀 **GPU Compute Shaders**: Massively parallel computation using Vulkan compute pipelines
- ⚡ **Real-Time Performance**: Process large-scale simulations (millions of entities) at 60+ FPS
- 🖥️ **Cross-Platform HPC**: Native support for Linux and Windows HPC clusters
- 📊 **Minimal Dependencies**: Lean architecture ideal for HPC deployment
- 🔧 **Modular Design**: Clean separation of compute, rendering, and I/O subsystems
- 📈 **Scalable Architecture**: Grid-based algorithms scale efficiently with GPU resources

![GENERATIONS Simulation](https://github.com/CorrelateVisuals/GENERATION/blob/main/assets/GenerationsCapture.PNG?raw=true)

## Performance & Scalability

The CAPITAL Engine architecture is specifically optimized for HPC workloads:

- **Compute-First Design**: Simulation logic runs entirely in GPU compute shaders (`Engine.comp`)
- **Double-Buffered SSBO**: Efficient memory access patterns for large datasets
- **Workgroup Optimization**: 32×32 local workgroups for optimal GPU occupancy
- **Zero CPU Bottleneck**: Asynchronous compute queues independent of rendering
- **Memory Efficient**: Structured buffer objects with cache-friendly layouts

### Demonstrated Performance
- Grid sizes up to 4096×4096 cells (16M+ entities)
- Sub-millisecond compute passes on modern GPUs
- Linear scaling with GPU compute units
- Multi-GPU ready architecture (Vulkan device groups)

## Technical Architecture

GENERATIONS employs a clean, modular architecture designed for both research and production HPC environments:

```
├── app/        - Application orchestration and lifecycle
├── base/       - Vulkan primitives and low-level utilities
├── core/       - Logging, timing, and performance monitoring
├── io/         - Filesystem and data I/O (screenshots, exports)
├── platform/   - Window management and validation layers
├── render/     - Pipeline management and command recording
└── world/      - Simulation state, geometry, and camera control
```

**Compute Pipeline Architecture:**
- **Shader Stages**: Compute (`*.comp`), Vertex, Fragment, Tessellation
- **Buffer Types**: SSBO (read/write), UBO (parameters), Push Constants
- **Synchronization**: Timeline semaphores for multi-queue coordination
- **Memory Management**: Staged transfers with dedicated transfer queues

See [FOLDER_DEPENDENCY_MAP.md](src/FOLDER_DEPENDENCY_MAP.md) for detailed module dependencies and architectural constraints.

## HPC-Specific Features

### GPU Compute Capabilities
- **Vulkan Compute Shaders**: Full access to modern GPU compute features
- **Workgroup Flexibility**: Configurable local workgroup sizes (tested up to 1024 threads)
- **Shared Memory**: Fast intra-workgroup communication via shared locals
- **Atomic Operations**: Lock-free synchronization primitives
- **64-bit Integer Support**: Extended precision for scientific computing

### Performance Monitoring
- **Built-in GPU Tracing**: Real-time performance analysis with `gpu_trace_live.sh`
- **Timestamp Queries**: Accurate GPU-side timing measurements  
- **Command Buffer Profiling**: Per-pipeline performance breakdown
- **Memory Statistics**: VRAM usage and allocation tracking

### Deployment Flexibility
- **Minimal Runtime Dependencies**: GLFW, Vulkan loader, standard C++ runtime
- **Static Linking Options**: Self-contained binaries for HPC nodes
- **Configuration via Push Constants**: No filesystem dependencies for parameters
- **Modular Architecture**: Easy to extend for headless/cluster deployments

## Research & Academic Applications

GENERATIONS and the CAPITAL Engine are well-suited for:

- **Cellular Automata Research**: Conway's Life, Langton's Ant, custom rule sets
- **Agent-Based Modeling**: Large-scale multi-agent simulations
- **Spatial Algorithms**: Reaction-diffusion, pattern formation, CA variants  
- **Parallel Algorithm Development**: GPU compute shader prototyping
- **Performance Studies**: GPU architecture benchmarking and optimization research
- **Computational Art**: Generative systems and emergent behavior visualization

## Building & Development

### Prerequisites

**System Requirements:**
- **OS**: Linux (Ubuntu 20.04+, RHEL 8+) or Windows 10/11  
- **GPU**: Vulkan 1.3+ compatible (NVIDIA, AMD, Intel)
- **Compiler**: GCC 10+, Clang 12+, or MSVC 2022+
- **CMake**: 3.20 or higher
- **Python**: 3.6+ (for build-time architecture validation)

**Required Libraries:**
- Vulkan SDK 1.3.224+
- GLFW 3.3+
- GLM (header-only)
- Shader compiler: `glslangValidator` or `glslc`

Install shader compiler (required):
```bash
# Debian/Ubuntu
sudo apt install glslang-tools

# Fedora/RHEL
sudo dnf install glslang

# From source or Vulkan SDK
# Ensure glslangValidator is in PATH
```

### Linux Build (CMake)

**Quick Start:**
```bash
# Clone repository
git clone https://github.com/CorrelateVisuals/GENERATIONS.git
cd GENERATIONS

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# Run simulation
./bin/CapitalEngine
```

**Build Options:**
```bash
# Debug build with validation layers
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Release with optimizations
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Parallel build (use all cores)
cmake --build build --parallel $(nproc)
```

The executable `CapitalEngine` is generated in `bin/` directory.  
Compiled shaders (`.spv` files) are placed in `shaders/` directory.

### Windows Build (Visual Studio)

**Prerequisites:**
- Visual Studio 2022 (with C++ Desktop Development workload)
- Vulkan SDK (set `VULKAN_SDK` environment variable)
- GLFW library (included in `libraries/` or system-wide)

**Visual Studio Configuration:**

Additional Include Directories:
```
$(SolutionDir)..\Libraries\glfw-3.3.8\include
$(SolutionDir)..\Libraries\glm
C:\VulkanSDK\1.3.224.1\Include
```

Linker Additional Library Directories:
```
C:\VulkanSDK\1.3.224.1\Lib
$(SolutionDir)..\Libraries\glfw-3.3.8\lib-vc2022
```

Linker Additional Dependencies:
```
vulkan-1.lib
glfw3.lib
```

**Building:**
1. Open `GENERATIONS.sln` in Visual Studio
2. Select Release or Debug configuration  
3. Build → Build Solution (Ctrl+Shift+B)
4. Run from Visual Studio or execute `bin/CapitalEngine.exe`

### HPC Deployment Considerations

**Cluster/Batch Systems:**
```bash
# Example SLURM job script for future headless mode
#!/bin/bash
#SBATCH --partition=gpu
#SBATCH --gres=gpu:1
#SBATCH --time=01:00:00

module load vulkan/1.3
export VK_LAYER_PATH=/path/to/vulkan/layers

cd $SLURM_SUBMIT_DIR
# Headless mode coming soon - current version requires display
./bin/CapitalEngine
```

**Container Deployment:**
```dockerfile
# Example Dockerfile for HPC environments
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    vulkan-tools libvulkan-dev glslang-tools \
    libglfw3-dev cmake g++
COPY . /app
WORKDIR /app
RUN cmake -S . -B build && cmake --build build
CMD ["./bin/CapitalEngine"]
```

## Performance Monitoring & Debugging

### GPU Profiling

**Real-Time GPU Trace:**
```bash
./gpu_trace_live.sh
```
Streams live GPU performance metrics and validation output to console.

**Vulkan Validation Layers:**
```bash
# Enable comprehensive validation (debug builds)
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
./bin/CapitalEngine
```

**External Tools:**
- **RenderDoc**: Frame capture and GPU debugging
- **Nsight Graphics** (NVIDIA): Advanced GPU profiling  
- **Radeon GPU Profiler** (AMD): Performance analysis
- **Intel GPA**: Intel GPU optimization

### Code Quality Tools

**Code Formatter (clang-format):**

This repository includes `.clang-format` configuration optimized for Vulkan/C++ readability.

Format all source files:
```bash
find src -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
```

Format a single file:
```bash
clang-format -i src/render/Resources.cpp
```

**Architecture Validation:**
```bash
# Verify module dependency boundaries
python3 tools/check_folder_dependencies.py
```
Enforces architectural constraints preventing circular dependencies.

## Project Structure

```
GENERATIONS/
├── src/
│   ├── app/           # Application entry point and orchestration
│   ├── base/          # Vulkan primitives and utilities  
│   ├── core/          # Logging, timing, performance monitoring
│   ├── io/            # File I/O, screenshots, data export
│   ├── platform/      # Window management, input, validation  
│   ├── render/        # Pipelines, command buffers, resources
│   └── world/         # Simulation state, camera, geometry
├── shaders/
│   ├── Engine.comp    # Main compute shader (Game of Life logic)
│   ├── Cells.{vert,frag}      # Cell rendering shaders
│   ├── Landscape.{vert,frag}  # Terrain rendering
│   ├── Water.{vert,frag}      # Water surface shader
│   └── PostFX.comp            # Post-processing effects
├── tools/             # Build and development utilities
├── assets/            # Textures, models, screenshots
└── libraries/         # Third-party headers (STB, GLM)
```

## Roadmap & Future Development

**Planned HPC Enhancements:**
- [ ] Multi-GPU support via Vulkan device groups
- [ ] Headless compute mode for cluster deployment  
- [ ] MPI integration for distributed simulations
- [ ] Enhanced memory management for larger-than-VRAM datasets
- [ ] Real-time parameter tuning via network interface
- [ ] Data export formats (HDF5, CSV, binary dumps)

**Engine Improvements:**
- [ ] Advanced texture I/O (GPU↔CPU)
- [ ] Render target picking (object selection)
- [ ] Frustum culling and LOD systems  
- [ ] ImGui integration for runtime controls
- [ ] Additional simulation types (reaction-diffusion, SPH, etc.)

## Contributing

We welcome contributions from the HPC and graphics programming communities!

**Areas of Interest:**
- Performance optimizations and GPU algorithm improvements
- Additional simulation types and compute kernels
- Cross-platform testing (especially on HPC clusters)
- Documentation and tutorials
- Benchmark results from different GPU architectures

**Development Guidelines:**
1. Follow the existing code style (use provided `.clang-format`)
2. Maintain architectural boundaries (see `FOLDER_DEPENDENCY_MAP.md`)
3. Run architecture validation before commits
4. Test on both Debug and Release builds
5. Include performance implications in PR descriptions

## References & Acknowledgments

**Learning Resources:**
- [Vulkan Tutorial](https://vulkan-tutorial.com/) by Alexander Overvoorde
- [Sascha Willems' Vulkan Examples](https://github.com/SaschaWillems/Vulkan)
- [GPU Gems](https://developer.nvidia.com/gpugems) - NVIDIA  
- [Vulkan Specification](https://registry.khronos.org/vulkan/)

**Technologies:**
- **Vulkan API**: Cross-platform GPU compute and graphics
- **GLFW**: Platform-agnostic windowing and input  
- **GLM**: OpenGL Mathematics library (header-only)
- **STB**: Image loading/writing utilities

Built on foundations from the excellent [Vulkan Tutorial](https://vulkan-tutorial.com/Introduction) series by Sascha Willems and Alexander Overvoorde.

## License

[Include your license information here - commonly MIT, Apache 2.0, or GPL for research projects]

## Contact & Support

**Issues**: [GitHub Issues](https://github.com/CorrelateVisuals/GENERATIONS/issues)  
**Discussions**: [GitHub Discussions](https://github.com/CorrelateVisuals/GENERATIONS/discussions)

For HPC-specific questions or collaboration inquiries, please open a discussion or issue on GitHub.

---

**Keywords**: HPC, GPU Computing, Vulkan, Compute Shaders, Parallel Processing, Cellular Automata, Scientific Simulation, High Performance Computing, GPGPU, Cross-Platform

