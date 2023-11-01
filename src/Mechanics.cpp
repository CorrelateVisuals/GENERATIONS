#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Mechanics.h"

VulkanMechanics::VulkanMechanics()
    : initVulkan{},
      mainDevice{initVulkan, queues, swapchain},
      queues{},
      swapchain{},
      syncObjects{} {
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
  Log::text(Log::Style::headerGuard);
  Log::text("{ Vk. }", "Setup Vulkan");

  swapchain.create(initVulkan.surface, queues, MAX_FRAMES_IN_FLIGHT);
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
  swapchain.destroy();
  syncObjects.destroy(MAX_FRAMES_IN_FLIGHT);
  mainDevice.destroyDevice();
}

void VulkanMechanics::Swapchain::recreate(const VkSurfaceKHR& surface,
                                          const CE::Queues& queues,
                                          SynchronizationObjects& syncObjects,
                                          Pipelines& _pipelines,
                                          Resources& _resources) {
  CE::Swapchain::recreate(surface, queues, syncObjects, MAX_FRAMES_IN_FLIGHT);
  _resources.msaaImage.createColorResources(extent, imageFormat);
  _resources.depthImage.createDepthResources(extent,
                                             CE::Image::findDepthFormat());
  _resources.createFramebuffers(_pipelines);
  _resources.createDescriptorSets();
}
