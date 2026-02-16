# Itterations-Dev Branch Setup

This document describes the setup and contents of the `itterations-dev` branch.

## What Was Done

### 1. Branch Creation
Created a new branch `itterations-dev` from the `copilot/add-agents-to-docker` branch.

### 2. Docker Support Integration  
Merged comprehensive Docker support from PR #51 including:
- Multi-architecture Dockerfiles (x86_64, ARM64)
- Docker Compose configuration with GPU profiles (NVIDIA, Mesa, Raspberry Pi)
- Helper scripts (`run-docker.sh`, `test-docker.sh`)
- Development container support
- Complete documentation (DOCKER.md, DOCKER_ARCHITECTURE.md, RASPBERRY_PI.md)

### 3. Agents/Tools from linux-script-cpp
Added analysis and monitoring tools:
- **gpu_trace_live.sh**: Live GPU tracing utility for monitoring buffer transfers and synchronization
- **analyze_colors.py**: Screenshot analysis tool for finding distinct object colors
- **analyze_water.py**: Tool for analyzing cells on water-colored terrain

### 4. Code Fixes
Fixed multiple compilation issues:
- Added missing `getBindingDescription()` methods to `World::Grid` and `Shape` classes
- Restored corrupted `Geometry.cpp` file
- Fixed naming convention inconsistencies (camelCase vs snake_case) throughout the codebase
- Ensured all files use snake_case naming to match the project's coding standards

## How to Use

### Running with Docker

The easiest way to run GENERATIONS is using Docker:

```bash
# For AMD/Intel/Mesa GPUs
./run-docker.sh mesa

# For NVIDIA GPUs (requires nvidia-container-toolkit)
./run-docker.sh nvidia

# For Raspberry Pi/ARM64
./run-docker.sh raspberry-pi

# For development (with full build tools)
./run-docker.sh dev
```

### Using the Analysis Tools

```bash
# Monitor GPU operations in real-time
./tools/gpu_trace_live.sh

# Analyze screenshot colors
python3 ./tools/analyze_colors.py

# Analyze water terrain
python3 ./tools/analyze_water.py
```

### Building from Source

See DOCKER.md for detailed build and setup instructions.

## Architecture

The codebase now has a modular structure with:
- **src/app/**: Application entry point
- **src/base/**: Vulkan abstraction layer
- **src/core/**: Core utilities (logging, timing)
- **src/io/**: I/O operations (file loading, screenshots)
- **src/platform/**: Platform-specific code (window, validation)
- **src/render/**: Rendering pipeline
- **src/world/**: World geometry and simulation

## Testing

Test the Docker build:
```bash
./test-docker.sh
```

## Notes

- The Docker setup uses host GPU drivers via device passthrough
- No drivers are installed in the container, making it robust to host driver updates
- The runtime image is ~173MB compressed
- Supports Vulkan GPU acceleration on x86_64 and ARM64 platforms
