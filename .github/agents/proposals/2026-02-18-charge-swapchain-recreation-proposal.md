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
    throw; // Re-throw to ensure proper error handling upstream
  }
}
```

## Vulkan Guru
Proposal

```cpp
// src/vulkan_mechanics/Mechanics.cpp

void Swapchain::recreate(VkSurfaceKHR surface,
                         const Queues &queues,
                         const SyncObjects &sync_objects,
                         Pipelines &pipelines,
                         VulkanResources &resources) {
  // Ensure proper cleanup of old swapchain resources
  vkDeviceWaitIdle(device);

  // Destroy old swapchain images and framebuffers
  for (auto framebuffer : framebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }
  framebuffers.clear();

  for (auto imageView : imageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  imageViews.clear();

  vkDestroySwapchainKHR(device, swapchain, nullptr);

  // Recreate swapchain
  create_swapchain(surface);
  create_image_views();
  create_framebuffers();

  // Update dependent resources
  pipelines.update_for_new_swapchain();
  resources.update_bindings_for_swapchain();
}
```

---
