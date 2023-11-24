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
                                          Pipelines& pipelines,
                                          Resources& resources) {
  CE::Swapchain::recreate(surface, queues, syncObjects);
  resources.msaaImage.createResources(extent, imageFormat,
                                      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
  resources.depthImage.createResources(
      extent, CE::Image::findDepthFormat(),
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
  pipelines.render.createFramebuffers(*this, resources.msaaImage.view,
                                      resources.depthImage.view);
  CE::Descriptor::createSets(images);
}
