# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
  try {
    Log::text(Log::Style::header_guard);
    Log::text("| Recreating Swapchain");

    // Log the current state before recreation
    Log::text("Current Swapchain State:");
    Log::text("  - Last Presented Image Index: " + std::to_string(last_presented_image_index));
    Log::text("  - Last Submitted Frame Index: " + std::to_string(last_submitted_frame_index));

    // Perform swapchain recreation
    mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                 mechanics.queues,
                                 mechanics.sync_objects,
                                 *pipelines,
                                 *resources);

    // Log success
    Log::text("| Swapchain successfully recreated");
    Log::text(Log::Style::header_guard);
  } catch (const std::exception &e) {
    // Log the error and rethrow
    Log::text(Log::Style::header_guard);
    Log::text("| ERROR: Swapchain recreation failed: " + std::string(e.what()));
    Log::text(Log::Style::header_guard);
    throw;
  }
}
```

## Vulkan Guru
Proposal

```cpp
// src/vulkan_mechanics/Mechanics.cpp

void Mechanics::recreate_swapchain(VkSurfaceKHR surface,
                                   const Queues &queues,
                                   SyncObjects &sync_objects,
                                   Pipelines &pipelines,
                                   VulkanResources &resources) {
  // Destroy old swapchain resources
  vkDeviceWaitIdle(device); // Ensure no resources are in use
  cleanup_swapchain_resources();

  // Recreate swapchain
  swapchain.create(surface, queues, sync_objects);

  // Recreate dependent resources
  resources.recreate_for_swapchain(swapchain);
  pipelines.recreate_for_swapchain(swapchain)

... [output truncated by factory gate] ...
