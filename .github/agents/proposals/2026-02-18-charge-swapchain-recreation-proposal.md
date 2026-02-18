# Code Proposal — CHARGE-SWAPCHAIN-RECREATION (2026-02-18)

## C++ Lead
Proposal

```cpp
// src/engine/CapitalEngine.cpp

void CapitalEngine::recreate_swapchain() {
  // Log the start of the swapchain recreation process
  Log::text(Log::Style::header_guard);
  Log::text("| Recreating Swapchain");

  // Destroy dependent Vulkan resources before recreating the swapchain
  pipelines->destroy();
  resources->destroy();

  // Recreate the swapchain
  mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                               mechanics.queues,
                               mechanics.sync_objects,
                               *pipelines,
                               *resources);

  // Reinitialize dependent Vulkan resources
  resources->initialize(mechanics.swapchain);
  pipelines->initialize(mechanics.swapchain);

  // Log the completion of the swapchain recreation process
  Log::text("| Swapchain recreation complete");
  Log::text(Log::Style::header_guard);
}
```

### Key Changes:
1. Added logging to track the start and end of the swapchain recreation process.
2. Explicitly destroyed and reinitialized Vulkan resources (`pipelines` and `resources`) to ensure proper cleanup and reallocation.
3. Improved clarity by separating resource destruction and initialization steps.

## Vulkan Guru
Proposal

```cpp
// src/vulkan_mechanics/Mechanics.cpp

void Swapchain::recreate(VkSurfaceKHR surface,
                         const Queues &queues,
                         const SyncObjects &sync_objects,
                         Pipelines &pipelines,
                         VulkanResources &resources) {
  // Ensure all GPU operations are complete before destroying resources
  vkDeviceWaitIdle(device);

  // Destroy old swapchain resources
  destroy_swapchain_resources();

  // Query surface capabilities and formats
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

  VkSurfaceFormatKHR surface_format = choose_surface_format(surface_formats);
  VkExtent2D extent = choose_swap_extent(surface_capabilities);

  // Recreate the swapchain
  create_swapchain(surface, surface_format, extent);

  // Recreate dependent resources
  resources.recreate_for_swapchain(swapchain, extent, surface_format.format);
  pipelines.recreate_for_swapchain(swapchain, extent, surface_format.format);

  // Recreate synchronization objects
  sync_objects.recreate_for_swapchain(swapchain_image_count);

  // Log the completion of the swapchain recreation process
  Log::text("| Swapchain recreation complete");
}
```
