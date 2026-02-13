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
    resources.msaa_image.create_resources(CE_MULTISAMPLE_IMAGE, extent, image_format);
    resources.depth_image.create_resources(
      CE_DEPTH_IMAGE, extent, CE::Image::find_depth_format());
  pipelines.render.create_framebuffers(
      *this, resources.msaa_image.view, resources.depth_image.view);

  resources.storage_image.create_descriptor_write(resources.descriptor_interface, images);
  resources.descriptor_interface.update_sets();
}
