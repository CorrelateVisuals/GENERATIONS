# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
void CapitalEngine::recreate_swapchain() {
    // Log the start of the swapchain recreation process
    Log::text(Log::St

... [output truncated by factory gate] ...

## Vulkan Guru
Proposal
```cpp
void CapitalEngine::recreate_swapchain() {
    // Log the start of the swapchain recreation process
    Log::text(Log::Style::header_guard);
    Log::text("| Recreating Swapchain");

    // Cleanup resources dependent on the swapchain
    frame_context->cleanup();
    pipelines->cleanup();
    resources->cleanup();

    // Recreate the swapchain
    try {
        mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                     mechanics.queues,
                                     mechanics.sync_objects,
                                     *pipelines,
                                     *resources);
    } catch (const std::exception& e) {
        Log::text(Log::Style::header_guard);
        Log::text("| Swapchain recreation failed: ", e.what());
        throw; // Re-throw the exception to allow higher-level handling
    }

    // Reinitialize resources dependent on the new swapchain
    resources->initialize(mechanics.swapchain);
    pipelines->initialize(mechanics.swapchain, *resources);
    frame_context->initialize(mechanics.swapchain, *resources, *pipelines);

    // Log the successful completion of the swapchain recreation process
    Log::text("| Swapchain recreation completed successfully");
    Log::text(Log::Style::header_guard);
}
```
