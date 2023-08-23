# G E N E R A T I O N S

GENERATIONS is a civilization simulator build on top of CAPITAL Engine a cross platform Vulkan engine. Built for simulations and algorithms that benefit from parallel computing. Currently tested on Linux and Windows, keeping external libraries to a minimum. GLFW for platform agnostic window and input handeling, GLM for parallel computations. 

Currently GENERATIONS is running ![Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).

![Cover Image](https://raw.githubusercontent.com/CorrelateVisuals/GENERATIONS/main/assets/GenerationsCapture.PNG?token=GHSAT0AAAAAACFIA634CAOQ74BUN3224FLKZHFJ7UQ)

## Under development
- Arcball Camera for improved camera control.
- Integration of Dear ImGui UI library for user interfaces.
- Color picking functionality for screenspace position to vertex ID conversion.
- Geometry loading
- Compute shader-based culling and level-of-detail (LOD) techniques for optimized rendering.
- Addition of a tesselation shader stage for geometry processing and manipulation.

[Development enviroment](https://vulkan-tutorial.com/Development_environment)

## Windows development
Additional Include Directories
```
$(SolutionDir)..\Libraries\glfw-3.3.8\include
$(SolutionDir)..\Libraries\glm
C:\VulkanSDK\1.3.224.1\Include
```
Linker Additional Libraries Directories
```text
C:\VulkanSDK\1.3.224.1\Lib
$(SolutionDir)..\Libraries\glfw-3.3.8\lib-vc2022
```
Linker Additional Dependencies
```text
vulkan-1.lib
glfw3.lib
```

## Linux development
Build management using CMake
Go to **build** sub-directory:
```bash
  cmake ..
  make -j
```
The excutable **CapitalEngine** is compiled in the **bin** sub-directory. De .spv files in **shaders** sub-directory.
Executing: Go to the project root directory **GENERATION**:
```bash
./bin/CapitalEngine
```



Started from the tutorial series by *Sascha Willems*: [Vulkan tutorial](https://vulkan-tutorial.com/Introduction).

