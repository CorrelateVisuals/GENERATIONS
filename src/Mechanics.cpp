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
  _resources.msaaImage.createResources(extent, imageFormat,
                                       VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT);
  _resources.depthImage.createResources(
      extent, CE::Image::findDepthFormat(),
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
  _resources.createFramebuffers(_pipelines);
  _resources.createDescriptorSets();
}
