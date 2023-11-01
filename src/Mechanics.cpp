#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Mechanics.h"

VulkanMechanics::VulkanMechanics()
    : initVulkan{},
      mainDevice{initVulkan, queues, swapchain},
      queues{},
      swapchain{initVulkan.surface, queues},
      syncObjects{} {
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
  Log::text(Log::Style::headerGuard);
  Log::text("{ Vk. }", "Setup Vulkan");
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
}

void VulkanMechanics::Swapchain::recreate(const VkSurfaceKHR& surface,
                                          const CE::Queues& queues,
                                          SynchronizationObjects& syncObjects,
                                          Pipelines& _pipelines,
                                          Resources& _resources) {
  CE::Swapchain::recreate(surface, queues, syncObjects);
  _resources.msaaImage.createColorResources(extent, imageFormat);
  _resources.depthImage.createDepthResources(extent,
                                             CE::Image::findDepthFormat());
  _resources.createFramebuffers(_pipelines);
  _resources.createDescriptorSets();
}
