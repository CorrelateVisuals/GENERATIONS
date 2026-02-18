# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
  // Log the start of the swapchain recreation process
  Log::text(Log::Style::header_guard);
  Log::text("| Recreating Swapchain");

  // Ensure Vulkan resources are properly cleaned up before recreating the swapchain
  mechanics.swapchain.recreate(
      mechanics.init_vulkan.surface,
      mechanics.queues,
      mechanics.sync_objects,
      *pipelines,
      *resources);

  // Log the completion of the swapchain recreation process
  Log::text("| Swapchain Recreation Complete");
  Log::text(Log::Style::header_guard);
}
```

<reasoning>
The proposed change ensures that the `recreate_swapchain` function logs the start and end of the swapchain recreation process, improving traceability during debugging. The function itself remains minimal and delegates the actual Vulkan resource handling to the `mechanics.swapchain.recreate` method, adhering to the principle of clear ownership.

The Vulkan Guru will need to confirm that the `recreate` method in `Mechanics.cpp` properly handles Vulkan resource cleanup and reallocation. Additionally, synchronization objects and pipeline integration must be validated to ensure correctness.

The Kernel Expert will need to verify that shaders are compatible with any changes to swapchain dimension

... [output truncated by factory gate] ...

## Vulkan Guru
Proposal

```cpp
// src/vulkan_mechanics/Mechanics.cpp

void VulkanMechanics::recreate_swapchain(VkSurfaceKHR surface, const Queues& queues, const SyncObjects& sync_objects, Pipelines& pipelines, VulkanResources& resources) {
    // Ensure all GPU operations are complete before destroying resources
    vkDeviceWaitIdle(device);

    // Destroy old swapchain images and associated resources
    swapchain.cleanup();

    // Recreate the swapchain
    swapchain.create(surface, queues, sync_objects);

    // Reintegrate new swapchain images with pipelines and descriptor sets
    pipelines.update_for_swapchain(swapchain);
    resources.update_for_swapchain(swapchain);

    // Log the completion of the swapchain recreation process
    Log::text(Log::Style::header_guard);
    Log::text("| Swapchain recreation complete");
}
```
