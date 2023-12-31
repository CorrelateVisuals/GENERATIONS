# GENERATIONS
GENERATIONS is an economical simulator build on top of CAPITAL Engine. A cross platform Vulkan engine, built for simulations and algorithms that benefit from parallel computing. Tested on Linux and Windows, keeping external libraries to a minimum. Using GLFW for platform agnostic window and input handeling and GLM for convenient typedefs. 

Currently GENERATIONS is running ![Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).

![Cover Image](https://github.com/CorrelateVisuals/GENERATION/blob/main/assets/GenerationsCapture.PNG?raw=true)

### Expanding Functionality
- Texture/Image reading and writing GPU & CPU
- Color picking/Render picking
- Culling and level-of-detail (LOD)
- Integrating GUI (Dear ImGui)
- Arcball Camera

[Development enviroment](https://vulkan-tutorial.com/Development_environment)

## Dependencies
You will need glslangValidator installed and added to your systempaths. So that it can be ran from the command line.

### Windows development
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

### Linux development
Build management using CMake
Go to **build** sub-directory:
```bash
  cmake ..
  make -j
```
The excutable **CapitalEngine** is compiled in the **bin** sub-directory. De .spv files in **shaders** sub-directory.
Executing: Go to the project root directory **GENERATIONS**:
```bash
./bin/CapitalEngine
```



Build on the tutorial series by *Sascha Willems*: [Vulkan tutorial](https://vulkan-tutorial.com/Introduction).

