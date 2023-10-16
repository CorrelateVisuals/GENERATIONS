#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Mechanics.h"

VulkanMechanics::VulkanMechanics()
    : initVulkan{}, mainDevice{}, queues{}, swapchain{}, syncObjects{} {
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
  Log::text(Log::Style::headerGuard);
  Log::text("{ Vk. }", "Setup Vulkan");

  mainDevice.pickPhysicalDevice(initVulkan, queues, swapchain);
  mainDevice.createLogicalDevice(initVulkan, queues);
  CE::baseDevice->setBaseDevice(mainDevice);

  swapchain.create(initVulkan.surface, queues);
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
  swapchain.destroy();
  syncObjects.destroy(MAX_FRAMES_IN_FLIGHT);
  mainDevice.destroyDevice();
}

void VulkanMechanics::Swapchain::recreate(const VkSurfaceKHR& surface,
                                          const Queues& queues,
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
