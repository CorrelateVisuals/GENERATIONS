#include "vulkan/vulkan.h"

#include "Mechanics.h"
#include "core/Log.h"
#include "render/Pipelines.h"
#include "render/Resources.h"

VulkanMechanics::VulkanMechanics()
    : initVulkan{}, queues{}, swapchainSupport{},
      mainDevice{initVulkan, queues, swapchainSupport}, swapchain{}, syncObjects{} {
  swapchain.initialize(initVulkan.surface, queues);
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
  Log::text(Log::Style::headerGuard);
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
}

void VulkanMechanics::Swapchain::recreate(const VkSurfaceKHR &surface,
                                          const CE::Queues &queues,
                                          SynchronizationObjects &syncObjects,
                                          Pipelines &pipelines,
                                          Resources &resources) {
  CE::Swapchain::recreate(surface, queues, syncObjects);
    resources.msaaImage.create_resources(CE_MULTISAMPLE_IMAGE, extent, image_format);
    resources.depthImage.create_resources(
      CE_DEPTH_IMAGE, extent, CE::Image::find_depth_format());
  pipelines.render.create_framebuffers(
      *this, resources.msaaImage.view, resources.depthImage.view);

  resources.storageImage.createDescriptorWrite(resources.descriptorInterface, images);
  resources.descriptorInterface.update_sets();
}
