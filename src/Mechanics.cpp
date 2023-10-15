#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Mechanics.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>

VulkanMechanics::VulkanMechanics()
    : initVulkan{}, mainDevice{}, queues{}, swapchain{}, syncObjects{} {
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
  swapchain.destroy();
  syncObjects.destroy(MAX_FRAMES_IN_FLIGHT);
  mainDevice.destroyDevice();
}

void VulkanMechanics::setupVulkan(Pipelines& _pipelines,
                                  Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ Vk. }", "Setup Vulkan");

  pickPhysicalDevice(_resources.msaaImage.info.samples);
  mainDevice.createLogicalDevice(initVulkan, queues);
  CE::baseDevice->setBaseDevice(mainDevice);

  swapchain.create(initVulkan.surface, queues);
}

void VulkanMechanics::pickPhysicalDevice(
    VkSampleCountFlagBits& msaaImageSamples) {
  Log::text("{ ### }", "Physical Device");
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error(
        "\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      mainDevice.physical = device;
      mainDevice.getMaxUsableSampleCount();
      break;
    }
  }
  if (mainDevice.physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

bool VulkanMechanics::checkDeviceExtensionSupport(
    VkPhysicalDevice physicalDevice) {
  Log::text(Log::Style::charLeader, "Check Device Extension Support");
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(mainDevice.extensions.begin(),
                                           mainDevice.extensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

// VkSampleCountFlagBits VulkanMechanics::getMaxUsableSampleCount() {
//   VkPhysicalDeviceProperties physicalDeviceProperties;
//   vkGetPhysicalDeviceProperties(mainDevice.physical,
//   &physicalDeviceProperties); VkSampleCountFlags counts =
//       physicalDeviceProperties.limits.framebufferColorSampleCounts &
//       physicalDeviceProperties.limits.framebufferDepthSampleCounts;
//
//   for (size_t i = VK_SAMPLE_COUNT_64_BIT; i >= VK_SAMPLE_COUNT_1_BIT; i >>=
//   1) {
//     if (counts & i) {
//       return static_cast<VkSampleCountFlagBits>(i);
//     }
//   }
//   return VK_SAMPLE_COUNT_1_BIT;
// }

bool VulkanMechanics::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
  Log::text(Log::Style::charLeader, "Is Device Suitable");

  queues.familyIndices =
      queues.findQueueFamilies(physicalDevice, initVulkan.surface);
  bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

  bool swapchainAdequate = false;
  if (extensionsSupported) {
    Swapchain::SupportDetails swapchainSupport =
        swapchain.checkSupport(physicalDevice, initVulkan.surface);
    swapchainAdequate = !swapchainSupport.formats.empty() &&
                        !swapchainSupport.presentModes.empty();
  }

  // VkPhysicalDeviceFeatures supportedFeatures;
  // vkGetPhysicalDeviceFeatures(mainDevice.physical, &supportedFeatures);
  //&& supportedFeatures.samplerAnisotropy

  return queues.familyIndices.isComplete() && extensionsSupported &&
         swapchainAdequate;
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
