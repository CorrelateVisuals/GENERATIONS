#include "vulkan/vulkan.h"

#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"

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
  resources.msaaImage.createResources(CE_MULTISAMPLE_IMAGE, extent,
                                      imageFormat);
  resources.depthImage.createResources(CE_DEPTH_IMAGE, extent,
                                       CE::Image::findDepthFormat());
  pipelines.render.createFramebuffers(*this, resources.msaaImage.view,
                                      resources.depthImage.view);

  resources.descriptorInterface.updateSets();
}
