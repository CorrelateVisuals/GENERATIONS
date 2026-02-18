# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
  // Log the start of the swapchain recreation process
  Log::text(Log::Style::header_guard);
  Log::text("| Recreating Swapchain");

  // Ensure all GPU operations are complete before proceeding
  mechanics.device.wait_idle();

  // Cleanup resources tied to the old swapchain
  pipelines->cleanup();
  frame_context->cleanup();
  resources->cleanup();

  // Recreate the swapchain
  mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                               mechanics.queues,
     

... [output truncated by factory gate] ...

## Vulkan Guru
Proposal
```cpp
void CapitalEngine::recreate_swapchain() {
  // Ensure all GPU operations are completed before destroying resources
  vkDeviceWaitIdle(mechanics.device);

  // Destroy old swapchain-dependent resources
  resources->destroy_swapchain_dependent_resources();
  pipelines->destroy_swapchain_dependent_pipelines();

  // Recreate the swapchain
  mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                               mechanics.queues,
                               mechanics.sync_objects,
                               *pipelines,
                               *resources);

  // Reinitialize resources and pipelines for the new swapchain
  resources->initialize_swapchain_dependent_resources(mechanics.swapchain);
  pipelines->initialize_swapchain_dependent_pipelines(mechanics.swapchain);

  // Log the successful recreation
  Log::text("Swapchain successfully recreated.");
}
```
