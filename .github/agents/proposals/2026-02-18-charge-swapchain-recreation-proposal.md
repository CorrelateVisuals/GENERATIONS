# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
  try {
    Log::text("Recreating swapchain...");
    mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                 mechanics.queues,
                                 mechanics.sync_objects,
                                 *pipelines,
                                 *resources);
    Log::text("Swapchain recreation completed successfully.");
  } catch (const std::exception &e) {
    Log::text("Error during swapchain recreation: ", e.what());
    throw; // Re-throw to ensure proper error handling upstream.
  }
}
```

## Vulkan Guru
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
  try {
    Log::text("Recreating swapchain...");

    // Ensure all GPU operations are complete before destroying resources
    mechanics.sync_objects.wait_for_idle();

    // Destroy old swapchain resources
    mechanics.swapchain.cleanup();

    // Recreate the swapchain with updated surface and configuration
    mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                 mechanics.queues,
                                 mechanics.sync_objects,
                                 *pipelines,
                                 *resources);

    // Reinitialize dependent resources
    pipelines->rebuild_for_swapchain(mechanics.swapchain);
    resources->update_bindings_for_swapchain(mechanics.swapchain);

    Log::text("Swapchain recreation completed successfully.");
  } catch (const std::exception &e) {
    Log::text("Error during swapchain recreation: ", e.what());
    throw; // Re-throw to 

... [output truncated by factory gate] ...
