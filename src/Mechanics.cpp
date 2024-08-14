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
  resources._msaaImage.createResources(CE_MULTISAMPLE_IMAGE, extent,
                                       imageFormat);
  resources._depthImage.createResources(CE_DEPTH_IMAGE, extent,
                                        CE::Image::findDepthFormat());
  pipelines.render.createFramebuffers(*this, resources._msaaImage.view,
                                      resources._depthImage.view);

  resources._storageImage.create_descriptor_write(images);
  CE::Descriptor::updateSets(CE::Descriptor::sets,
                             CE::Descriptor::descriptorWrites);
}
