# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
    Log::text(Log::Style::header_guard);
    Log::text("[SWAPCHAIN] Starting swapchain recreation...");

    try {
        mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                     mechanics.queues,
                                     mechanics.sync_objects,
                                     *pipelines,
                                     *resources);

        Log::text("[SWAPCHAIN] Swapchain recreation completed successfully.");
    } catch (const std::exception &e) {
        Log::text("[SWAPCHAIN] Error during swapchain recreation: " + std::string(e.what()));
        throw; // Re-throw to ensure proper error handling upstream.
    }
}
```

## Vulkan Guru
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
    Log::text(Log::Style::header_guard);
    Log::text("[SWAPCHAIN] Starting swapchain recreation...");

    try {
        // Ensure proper cleanup of Vulkan resources tied to the old swapchain
        mechanics.swapchain.cleanup();

        // Recreate the swapchain with updated parameters
        mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                     mechanics.queues,
                                     mechanics.sync_objects,
                                     *pipelines,
                                     *resources);

        // Update image views and synchronization objects
        resources->update_image_views(mechanics.swapch

... [output truncated by factory gate] ...
