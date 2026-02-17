# Daily Repo Report - 20260217

## Git Summary
- Branch: dev-revert-main-linux
- Commit: cfa17bd
- Divergence (main...HEAD): 5/106
- Dirty files: 3

### Changed Files vs main
- .clang-format
- .gitattributes
- .github/workflows/architecture-check.yml
- .github/workflows/daily-repo-status.lock.yml
- .github/workflows/daily-repo-status.md
- .gitignore
- .vscode/c_cpp_properties.json
- .vscode/settings.json
- CMakeLists.txt
- CMakePresets.json
- GENERATIONS.sln
- LICENSE
- README.md
- assets/3D/Cube.mtl
- assets/3D/Rectangle.mtl
- assets/3D/Sphere.mtl
- assets/3D/SphereHR.mtl
- assets/3D/Torus.mtl
- assets/Tessellation.PNG
- assets/branding/cmake.svg
- assets/branding/cplusplus.svg
- assets/branding/vulkan.svg
- assets/economic/README.md
- assets/economic/definitions.md
- assets/economic/principles.md
- assets/economic/sources.md
- assets/timeline/Avatar.PNG
- assets/timeline/BirdsEye.PNG
- assets/timeline/CoverCapture.PNG
- assets/timeline/GenerationsCapture.PNG
- assets/timeline/LandscapeCapture.PNG
- assets/timeline/Nap.PNG
- assets/timeline/TerrainCover_20260215.png
- assets/timeline/TerrainRuntimeTopDown_20260215.png
- assets/timeline/TerrainRuntime_20260215.png
- assets/todo/README.md
- assets/tools/analyze_colors.py
- assets/tools/analyze_water.py
- assets/tools/check_folder_dependencies.py
- assets/tools/daily_report.sh
- assets/tools/generate_shader_interface.py
- assets/tools/gpu_trace_live.sh
- assets/tools/run_daily_report.sh
- assets/tools/stress_test.sh
- assets/viking_room.png
- build/.gitignore
- libraries/stb_image_write.h
- run.sh
- shaders/Cells.frag
- shaders/Cells.vert
- shaders/CellsFollower.vert
- shaders/ComputeCopy.comp
- shaders/ComputeInPlace.comp
- shaders/ComputeJitter.comp
- shaders/Cube.frag
- shaders/Cube.vert
- shaders/Engine.comp
- shaders/Landscape.frag
- shaders/Landscape.vert
- shaders/LandscapeDebug.frag
- shaders/LandscapeNormals.frag
- shaders/LandscapeShared.glsl
- shaders/LandscapeStage1.frag
- shaders/LandscapeStage2.frag
- shaders/LandscapeStatic.vert
- shaders/LandscapeWireFrame.frag
- shaders/LandscapeWireFrame.tesc
- shaders/LandscapeWireFrame.tese
- shaders/LandscapeWireFrame.vert
- shaders/ParameterUBO.glsl
- shaders/PostFX.comp
- shaders/PushConstants.glsl
- shaders/SeedCells.comp
- shaders/Sky.frag
- shaders/Sky.vert
- shaders/TerrainBox.frag
- shaders/TerrainBox.vert
- shaders/TerrainBoxSeam.vert
- shaders/TerrainField.glsl
- shaders/Texture.frag
- shaders/Texture.vert
- shaders/Water.frag
- shaders/Water.vert
- src/BaseClasses.cpp
- src/BaseClasses.h
- src/CAPITAL-engine.vcxproj
- src/CAPITAL-engine.vcxproj.filters
- src/Camera.cpp
- src/Camera.h
- src/CapitalEngine.cpp
- src/CapitalEngine.h
- src/Geometry.cpp
- src/Geometry.h
- src/Library.cpp
- src/Library.h
- src/Log.cpp
- src/Log.h
- src/Mechanics.cpp
- src/Mechanics.h
- src/Pipelines.cpp
- src/Pipelines.h
- src/Resources.cpp
- src/Resources.h
- src/ShaderAccess.cpp
- src/ShaderAccess.h
- src/Terrain.cpp
- src/Terrain.h
- src/Timer.cpp
- src/Timer.h
- src/ValidationLayers.cpp
- src/ValidationLayers.h
- src/Window.cpp
- src/Window.h
- src/World.cpp
- src/World.h
- src/control/Timer.cpp
- src/control/Timer.h
- src/control/Window.cpp
- src/control/Window.h
- src/control/gui.cpp
- src/control/gui.h
- src/engine/CapitalEngine.cpp
- src/engine/CapitalEngine.h
- src/engine/Log.cpp
- src/engine/Log.h
- src/library/Library.cpp
- src/library/Library.h
- src/library/Screenshot.cpp
- src/library/Screenshot.h
- src/main.cpp
- src/pch.h
- src/vulkan_base/VulkanBaseDescriptor.cpp
- src/vulkan_base/VulkanBaseDescriptor.h
- src/vulkan_base/VulkanBaseDevice.cpp
- src/vulkan_base/VulkanBaseDevice.h
- src/vulkan_base/VulkanBasePipeline.cpp
- src/vulkan_base/VulkanBasePipeline.h
- src/vulkan_base/VulkanBasePipelinePresets.h
- src/vulkan_base/VulkanBaseResources.cpp
- src/vulkan_base/VulkanBaseResources.h
- src/vulkan_base/VulkanBaseSync.cpp
- src/vulkan_base/VulkanBaseSync.h
- src/vulkan_base/VulkanBaseUtils.h
- src/vulkan_base/VulkanBaseValidationLayers.cpp
- src/vulkan_base/VulkanBaseValidationLayers.h
- src/vulkan_mechanics/Mechanics.cpp
- src/vulkan_mechanics/Mechanics.h
- src/vulkan_pipelines/FrameContext.cpp
- src/vulkan_pipelines/FrameContext.h
- src/vulkan_pipelines/Pipelines.cpp
- src/vulkan_pipelines/Pipelines.h
- src/vulkan_pipelines/ShaderAccess.cpp
- src/vulkan_pipelines/ShaderAccess.h
- src/vulkan_pipelines/ShaderInterface.h
- src/vulkan_resources/ParameterUBO.schema
- src/vulkan_resources/VulkanResources.cpp
- src/vulkan_resources/VulkanResources.h
- src/world/Camera.cpp
- src/world/Camera.h
- src/world/Geometry.cpp
- src/world/Geometry.h
- src/world/RuntimeConfig.cpp
- src/world/RuntimeConfig.h
- src/world/SceneConfig.cpp
- src/world/SceneConfig.h
- src/world/World.cpp
- src/world/World.h
- tools/result.txt
- tools/src_to_one_file.py

## Architecture Boundary Check
- Status: OK
\n\n<details><summary>Architecture Check Output</summary>\n\n```\nFolder dependency check passed.\n```\n</details>

## Modern C++ Heuristics
- Raw new/delete usage (src/):
src/vulkan_base/VulkanBaseDescriptor.h:30:  BaseDescriptorInterface(const BaseDescriptorInterface &) = delete;
src/vulkan_base/VulkanBaseDescriptor.h:31:  BaseDescriptorInterface &operator=(const BaseDescriptorInterface &) = delete;
src/vulkan_base/VulkanBaseDescriptor.h:32:  BaseDescriptorInterface(BaseDescriptorInterface &&) = delete;
src/vulkan_base/VulkanBaseDescriptor.h:33:  BaseDescriptorInterface &operator=(BaseDescriptorInterface &&) = delete;
src/vulkan_base/VulkanBaseResources.h:22:  BaseBuffer(const BaseBuffer &) = delete;
src/vulkan_base/VulkanBaseResources.h:23:  BaseBuffer &operator=(const BaseBuffer &) = delete;
src/vulkan_base/VulkanBaseResources.h:24:  BaseBuffer(BaseBuffer &&) = delete;
src/vulkan_base/VulkanBaseResources.h:25:  BaseBuffer &operator=(BaseBuffer &&) = delete;
src/vulkan_base/VulkanBaseResources.h:78:  BaseImage(const BaseImage &) = delete;
src/vulkan_base/VulkanBaseResources.h:79:  BaseImage &operator=(const BaseImage &) = delete;
src/vulkan_base/VulkanBaseResources.h:80:  BaseImage(BaseImage &&) = delete;
src/vulkan_base/VulkanBaseResources.h:81:  BaseImage &operator=(BaseImage &&) = delete;
src/vulkan_base/VulkanBaseDevice.h:49:  BaseInitializeVulkan(const BaseInitializeVulkan &) = delete;
src/vulkan_base/VulkanBaseDevice.h:50:  BaseInitializeVulkan &operator=(const BaseInitializeVulkan &) = delete;
src/vulkan_base/VulkanBaseDevice.h:51:  BaseInitializeVulkan(BaseInitializeVulkan &&) = delete;
src/vulkan_base/VulkanBaseDevice.h:52:  BaseInitializeVulkan &operator=(BaseInitializeVulkan &&) = delete;
src/vulkan_base/VulkanBaseDevice.h:70:  BaseDevice(const BaseDevice &) = delete;
src/vulkan_base/VulkanBaseDevice.h:71:  BaseDevice &operator=(const BaseDevice &) = delete;
src/vulkan_base/VulkanBaseDevice.h:72:  BaseDevice(BaseDevice &&) = delete;
src/vulkan_base/VulkanBaseDevice.h:73:  BaseDevice &operator=(BaseDevice &&) = delete;
src/vulkan_base/VulkanBaseValidationLayers.h:13:  BaseValidationLayers(const BaseValidationLayers &) = delete;
src/vulkan_base/VulkanBaseValidationLayers.h:14:  BaseValidationLayers &operator=(const BaseValidationLayers &) = delete;
src/vulkan_base/VulkanBaseValidationLayers.h:15:  BaseValidationLayers(BaseValidationLayers &&) = delete;
src/vulkan_base/VulkanBaseValidationLayers.h:16:  BaseValidationLayers &operator=(BaseValidationLayers &&) = delete;
src/vulkan_base/VulkanBaseSync.h:30:  BaseCommandBuffers(const BaseCommandBuffers &) = delete;
src/vulkan_base/VulkanBaseSync.h:31:  BaseCommandBuffers &operator=(const BaseCommandBuffers &) = delete;
src/vulkan_base/VulkanBaseSync.h:32:  BaseCommandBuffers(BaseCommandBuffers &&) = delete;
src/vulkan_base/VulkanBaseSync.h:33:  BaseCommandBuffers &operator=(BaseCommandBuffers &&) = delete;
src/vulkan_base/VulkanBaseSync.h:57:  BaseSingleUseCommands(const BaseSingleUseCommands &) = delete;
src/vulkan_base/VulkanBaseSync.h:58:  BaseSingleUseCommands &operator=(const BaseSingleUseCommands &) = delete;
src/vulkan_base/VulkanBaseSync.h:59:  BaseSingleUseCommands(BaseSingleUseCommands &&) = delete;
src/vulkan_base/VulkanBaseSync.h:60:  BaseSingleUseCommands &operator=(BaseSingleUseCommands &&) = delete;
src/vulkan_base/VulkanBaseSync.h:88:  BaseSynchronizationObjects(const BaseSynchronizationObjects &) = delete;
src/vulkan_base/VulkanBaseSync.h:89:  BaseSynchronizationObjects &operator=(const BaseSynchronizationObjects &) = delete;
src/vulkan_base/VulkanBaseSync.h:90:  BaseSynchronizationObjects(BaseSynchronizationObjects &&) = delete;
src/vulkan_base/VulkanBaseSync.h:91:  BaseSynchronizationObjects &operator=(BaseSynchronizationObjects &&) = delete;
src/vulkan_base/VulkanBaseSync.h:125:  BaseSwapchain(const BaseSwapchain &) = delete;
src/vulkan_base/VulkanBaseSync.h:126:  BaseSwapchain &operator=(const BaseSwapchain &) = delete;
src/vulkan_base/VulkanBaseSync.h:127:  BaseSwapchain(BaseSwapchain &&) = delete;
src/vulkan_base/VulkanBaseSync.h:128:  BaseSwapchain &operator=(BaseSwapchain &&) = delete;
src/vulkan_base/VulkanBasePipeline.h:38:  BasePipelineLayout(const BasePipelineLayout &) = delete;
src/vulkan_base/VulkanBasePipeline.h:39:  BasePipelineLayout &operator=(const BasePipelineLayout &) = delete;
src/vulkan_base/VulkanBasePipeline.h:40:  BasePipelineLayout(BasePipelineLayout &&) = delete;
src/vulkan_base/VulkanBasePipeline.h:41:  BasePipelineLayout &operator=(BasePipelineLayout &&) = delete;
src/vulkan_base/VulkanBasePipeline.h:53:  BaseRenderPass(const BaseRenderPass &) = delete;
src/vulkan_base/VulkanBasePipeline.h:54:  BaseRenderPass &operator=(const BaseRenderPass &) = delete;
src/vulkan_base/VulkanBasePipeline.h:55:  BaseRenderPass(BaseRenderPass &&) = delete;
src/vulkan_base/VulkanBasePipeline.h:56:  BaseRenderPass &operator=(BaseRenderPass &&) = delete;
src/vulkan_base/VulkanBasePipeline.h:82:  BasePipelinesConfiguration(const BasePipelinesConfiguration &) = delete;
src/vulkan_base/VulkanBasePipeline.h:83:  BasePipelinesConfiguration &operator=(const BasePipelinesConfiguration &) = delete;
src/vulkan_base/VulkanBasePipeline.h:84:  BasePipelinesConfiguration(BasePipelinesConfiguration &&) = delete;
src/vulkan_base/VulkanBasePipeline.h:85:  BasePipelinesConfiguration &operator=(BasePipelinesConfiguration &&) = delete;
src/vulkan_mechanics/Mechanics.h:14:  VulkanMechanics(const VulkanMechanics &) = delete;
src/vulkan_mechanics/Mechanics.h:15:  VulkanMechanics &operator=(const VulkanMechanics &) = delete;
src/vulkan_mechanics/Mechanics.h:16:  VulkanMechanics(VulkanMechanics &&) = delete;
src/vulkan_mechanics/Mechanics.h:17:  VulkanMechanics &operator=(VulkanMechanics &&) = delete;
src/vulkan_pipelines/Pipelines.h:19:	Pipelines(const Pipelines &) = delete;
src/vulkan_pipelines/Pipelines.h:20:	Pipelines &operator=(const Pipelines &) = delete;
src/vulkan_pipelines/Pipelines.h:21:	Pipelines(Pipelines &&) = delete;
src/vulkan_pipelines/Pipelines.h:22:	Pipelines &operator=(Pipelines &&) = delete;
src/library/Screenshot.h:17:  Screenshot() = delete;
src/library/Screenshot.h:18:  ~Screenshot() = delete;
src/control/Window.h:12:  Window(const Window &) = delete;
src/control/Window.h:13:  Window &operator=(const Window &) = delete;
src/control/Window.h:14:  Window(Window &&) = delete;
src/control/Window.h:15:  Window &operator=(Window &&) = delete;
src/vulkan_resources/VulkanResources.h:27:	VulkanResources(const VulkanResources &) = delete;
src/vulkan_resources/VulkanResources.h:28:	VulkanResources &operator=(const VulkanResources &) = delete;
src/vulkan_resources/VulkanResources.h:29:	VulkanResources(VulkanResources &&) = delete;
src/vulkan_resources/VulkanResources.h:30:	VulkanResources &operator=(VulkanResources &&) = delete;
src/engine/CapitalEngine.h:16:  CapitalEngine(const CapitalEngine &) = delete;
src/engine/CapitalEngine.h:17:  CapitalEngine &operator=(const CapitalEngine &) = delete;
src/engine/CapitalEngine.h:18:  CapitalEngine(CapitalEngine &&) = delete;
src/engine/CapitalEngine.h:19:  CapitalEngine &operator=(CapitalEngine &&) = delete;

- C allocation (src/):

- C-style cast (src/):
src/vulkan_base/VulkanBaseDevice.cpp:441:CE::BaseDevice::fill_queue_create_infos(const BaseQueues &queues) const {
src/vulkan_base/VulkanBaseDevice.cpp:482:CE::BaseDevice::fill_devices(const BaseInitializeVulkan &init_vulkan) const {
src/vulkan_base/VulkanBaseDevice.cpp:707:std::vector<const char *> CE::BaseInitializeVulkan::get_required_extensions() const {
src/vulkan_base/VulkanBaseDevice.h:29:    bool is_complete() const {
src/vulkan_base/VulkanBaseDevice.h:58:  std::vector<const char *> get_required_extensions() const;
src/vulkan_base/VulkanBaseDevice.h:97:  std::vector<VkDeviceQueueCreateInfo> fill_queue_create_infos(const BaseQueues &queues) const;
src/vulkan_base/VulkanBaseDevice.h:103:  std::vector<VkPhysicalDevice> fill_devices(const BaseInitializeVulkan &init_vulkan) const;
src/vulkan_base/VulkanBaseDevice.h:109:  bool check_device_extension_support(const VkPhysicalDevice &physical_device) const;
src/vulkan_base/VulkanBaseSync.h:145:  pick_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats) const;
src/vulkan_base/VulkanBaseSync.h:150:  uint32_t get_image_count(const BaseSwapchain::SupportDetails &swapchain_support) const;
src/vulkan_base/VulkanBaseSync.cpp:356:CE::BaseSwapchain::get_image_count(const BaseSwapchain::SupportDetails &swapchain_support) const {
src/vulkan_pipelines/FrameContext.cpp:51:  void maybe_log() const {
src/vulkan_pipelines/ShaderInterface.h:7:struct alignas(16) ParameterUBO {
src/CAPITAL-engine.vcxproj:74:    <OutDir>$(SolutionDir)src\x64\$(Configuration)\</OutDir>
src/CAPITAL-engine.vcxproj:79:    <OutDir>$(SolutionDir)src\x64\$(Configuration)\</OutDir>
src/CAPITAL-engine.vcxproj:84:    <OutDir>$(SolutionDir)src\x64\$(Configuration)\</OutDir>
src/CAPITAL-engine.vcxproj:89:    <OutDir>$(SolutionDir)src\x64\$(Configuration)\</OutDir>
src/control/Timer.h:14:  float get_day_fraction() const;
src/control/Window.h:55:  bool is_escape_pressed() const {
src/control/Timer.cpp:7:float Timer::get_day_fraction() const {
src/world/SceneConfig.cpp:280:void SceneConfig::apply_to_runtime() const {
src/world/Geometry.h:41:  bool operator==(const Vertex &other) const {
src/world/World.h:26:	struct alignas(16) Cell {
src/world/RuntimeConfig.h:36:// Any other value (including nullptr) returns false.
src/world/Geometry.cpp:176:  size_t operator()(const Vertex &vertex) const {
src/world/SceneConfig.h:18:  void apply_to_runtime() const;
src/world/Camera.h:30:  Mode get_mode() const {
src/world/Camera.h:49:  bool get_arcball_horizon_lock() const {
src/world/Camera.h:116:  alignas(16) glm::mat4 model{};
src/world/Camera.h:117:  alignas(16) glm::mat4 view{};
src/world/Camera.h:118:  alignas(16) glm::mat4 projection{};

## Recent Shader Interface
\n```\n# type cxx_name glsl_name [= default...]
vec4 light light
ivec2 grid_xy gridXY
float water_threshold waterThreshold
float cell_size cellSize
vec4 water_rules waterRules = 2.4 1.2 0.08 0.0
mat4 model model
mat4 view view
mat4 projection projection\n```
