#include "vulkan/vulkan.h"

#include "Mechanics.h"
#include "core/Log.h"
#include "render/Pipelines.h"
#include "render/Resources.h"

VulkanMechanics::VulkanMechanics()
    : init_vulkan{}, queues{}, swapchain_support{},
      main_device{init_vulkan, queues, swapchain_support}, swapchain{}, sync_objects{} {
  swapchain.initialize(init_vulkan.surface, queues);
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
  Log::text(Log::Style::header_guard);
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
}

void VulkanMechanics::Swapchain::recreate(const VkSurfaceKHR &surface,
                                          const CE::queues &queues,
                                          SynchronizationObjects &sync_objects,
                                          Pipelines &pipelines,
                                          Resources &resources) {
  CE::Swapchain::recreate(surface, queues, sync_objects);
    resources.msaa_image.create_resources(CE_MULTISAMPLE_IMAGE, extent, image_format);
    resources.depth_image.create_resources(
      CE_DEPTH_IMAGE, extent, CE::Image::find_depth_format());
  pipelines.render.create_framebuffers(
      *this, resources.msaa_image.view, resources.depth_image.view);

  resources.storage_image.create_descriptor_write(resources.descriptor_interface, images);
  resources.descriptor_interface.update_sets();
}
