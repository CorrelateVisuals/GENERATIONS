# Linux Setup Guide for GENERATIONS

This guide provides step-by-step instructions to compile and run GENERATIONS on Debian-based Linux systems (Ubuntu, Debian, Linux Mint, etc.).

## Prerequisites

GENERATIONS is a Vulkan-based graphics engine that requires specific development tools and libraries to build and run on Linux.

## Step-by-Step Installation

### Step 1: Install Required System Packages

Install all necessary development tools and libraries:

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libvulkan-dev \
    vulkan-tools \
    vulkan-validationlayers \
    spirv-tools \
    libglfw3-dev \
    libglm-dev \
    glslang-tools
```

**What this installs:**
- `build-essential`: C++ compiler (g++) and build tools
- `cmake`: Build system generator (version 3.20 or higher required)
- `git`: Version control (if cloning the repository)
- `libvulkan-dev`: Vulkan development headers and libraries
- `vulkan-tools`: Vulkan utilities including `vulkaninfo`
- `vulkan-validationlayers`: Vulkan validation layers for debugging
- `spirv-tools`: SPIR-V shader tools
- `libglfw3-dev`: GLFW library for window and input management
- `libglm-dev`: GLM mathematics library for graphics
- `glslang-tools`: Shader compiler including `glslangValidator` and `glslc`

### Step 2: Verify Vulkan Support

Check that your system supports Vulkan:

```bash
vulkaninfo --summary
```

If this command fails or shows no devices, you may need to install GPU-specific Vulkan drivers:

**For NVIDIA GPUs:**
```bash
sudo apt-get install nvidia-vulkan-driver
```

**For AMD GPUs:**
```bash
sudo apt-get install mesa-vulkan-drivers
```

**For Intel GPUs:**
```bash
sudo apt-get install mesa-vulkan-drivers intel-media-va-driver
```

### Step 3: Fix CMakeLists.txt Configuration

The CMakeLists.txt file currently references a non-existent `stb` directory. It needs to be updated to reference the correct `libraries` directory:

Edit `CMakeLists.txt` and change line 28 from:
```cmake
include_directories(${PROJECT_SOURCE_DIR}/stb)
```
to:
```cmake
include_directories(${PROJECT_SOURCE_DIR}/libraries)
```

Also update line 37 to remove the stb reference:
```cmake
file(GLOB_RECURSE CAPITALENGINE_SOURCES
    RELATIVE "${CMAKE_SOURCE_DIR}"
    src/*.cpp
    src/*.h
)
```

### Step 4: Build the Project

Navigate to the build directory and run CMake:

```bash
cd build
cmake ..
make -j$(nproc)
```

**Explanation:**
- `cmake ..`: Generates build files from CMakeLists.txt in the parent directory
- `make -j$(nproc)`: Compiles the project using all available CPU cores
- The executable `CapitalEngine` will be created in the `bin` directory
- Compiled shaders (.spv files) will be in the `shaders` directory

### Step 5: Run the Application

From the project root directory:

```bash
cd ..
./bin/CapitalEngine
```

The application should launch and display Conway's Game of Life simulation.

## Troubleshooting

### CMake Version Too Old

If you see an error about CMake version, you need CMake 3.20 or higher. Install a newer version:

```bash
# Remove old CMake
sudo apt-get remove cmake

# Install newer CMake via snap
sudo snap install cmake --classic
```

### Shader Compilation Errors

If shader compilation fails, ensure `glslc` is in your PATH:

```bash
which glslc
```

If not found, glslang-tools may not be properly installed. Reinstall it:

```bash
sudo apt-get install --reinstall glslang-tools
```

### Vulkan Initialization Errors

If the application fails to start with Vulkan errors:

1. Check Vulkan is working: `vulkaninfo`
2. Ensure validation layers are installed: `sudo apt-get install vulkan-validationlayers`
3. Check your GPU supports Vulkan
4. Install appropriate GPU drivers (see Step 2)

### Missing GLFW or GLM

If you see errors about missing GLFW or GLM headers:

```bash
sudo apt-get install libglfw3-dev libglm-dev
```

### Build Warnings

The project uses strict compiler warnings (-Wall -Wextra -Wpedantic). Some warnings may appear but shouldn't prevent compilation. If they do, you can temporarily reduce warning levels by editing CMakeLists.txt.

## Development Tips

### Clean Build

To perform a clean build:

```bash
cd build
rm -rf *
cmake ..
make -j$(nproc)
```

### Debug Build

For debugging, create a debug build:

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Check Dependencies

Verify all dependencies are installed:

```bash
# Check CMake
cmake --version

# Check compilers
g++ --version

# Check Vulkan
vulkaninfo --summary

# Check shader compiler
glslc --version
glslangValidator --version

# Check libraries
pkg-config --modversion glfw3
```

## System Requirements

- **OS**: Debian-based Linux (Ubuntu 20.04+, Debian 11+, etc.)
- **CMake**: Version 3.20 or higher
- **Compiler**: GCC 10+ or Clang 11+ with C++20 support
- **Graphics**: GPU with Vulkan 1.0+ support
- **RAM**: 2GB minimum
- **Disk**: 500MB for source code and build artifacts

## Additional Notes

- The project uses C++20 features, so a modern compiler is required
- Vulkan validation layers are helpful during development but can be disabled in production
- The application requires a windowing system (X11 or Wayland) to run
- For headless systems, additional configuration would be needed

## References

- [Vulkan Tutorial - Development Environment](https://vulkan-tutorial.com/Development_environment)
- [GLFW Documentation](https://www.glfw.org/documentation.html)
- [Vulkan SDK for Linux](https://vulkan.lunarg.com/sdk/home#linux)
