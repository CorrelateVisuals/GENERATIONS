#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Mechanics.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>

VulkanMechanics::VulkanMechanics()
    : surface(VK_NULL_HANDLE),
      instance(VK_NULL_HANDLE),
      mainDevice{},
      queues{std::nullopt, std::nullopt},
      swapchain{},
      syncObjects{} {
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");

  swapchain.destroy(mainDevice.logical);
  syncObjects.destroy(mainDevice.logical);
  mainDevice.destroyDevice();

  if (validation.enableValidationLayers) {
    validation.DestroyDebugUtilsMessengerEXT(
        instance, validation.debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

void VulkanMechanics::setupVulkan(Pipelines& _pipelines,
                                  Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ Vk. }", "Setup Vulkan");

  createInstance();
  validation.setupDebugMessenger(instance);
  createSurface(Window::get().window);

  pickPhysicalDevice(_resources.msaaImage.info.samples);
  createLogicalDevice();

  swapchain.create(mainDevice, surface);

  createCommandPool(&_resources.command.pool);
}

void VulkanMechanics::createInstance() {
  Log::text("{ VkI }", "Vulkan Instance");
  if (validation.enableValidationLayers &&
      !validation.checkValidationLayerSupport()) {
    throw std::runtime_error(
        "\n!ERROR! validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = Window::get().display.title,
                            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                            .pEngineName = "CAPITAL Engine",
                            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                            .apiVersion = VK_API_VERSION_1_3};
  Log::text(Log::Style::charLeader, appInfo.pApplicationName,
            appInfo.applicationVersion, "-", appInfo.pEngineName,
            appInfo.engineVersion, "-", "Vulkan", 1.3);

  auto extensions = getRequiredExtensions();

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validation.validation.size());
    createInfo.ppEnabledLayerNames = validation.validation.data();

    validation.populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }

  CE::vulkanResult(vkCreateInstance, &createInfo, nullptr, &instance);
}

void VulkanMechanics::createSurface(GLFWwindow* window) {
  Log::text("{ [ ] }", "Surface");
  CE::vulkanResult(glfwCreateWindowSurface, instance, window, nullptr,
                   &surface);
}

void VulkanMechanics::pickPhysicalDevice(
    VkSampleCountFlagBits& msaaImageSamples) {
  Log::text("{ ### }", "Physical Device");
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error(
        "\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      mainDevice.physical = device;
      msaaImageSamples = getMaxUsableSampleCount();
      break;
    }
  }
  if (mainDevice.physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

CE::Queues::FamilyIndices VulkanMechanics::findQueueFamilies(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface) {
  Log::text(Log::Style::charLeader, "Find Queue Families");

  CE::Queues::FamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.graphicsAndComputeFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &presentSupport);

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

VulkanMechanics::Swapchain::SupportDetails
VulkanMechanics::Swapchain::checkSupport(VkPhysicalDevice physicalDevice,
                                         VkSurfaceKHR& surface) {
  Log::text(Log::Style::charLeader, "Query Swap Chain Support");
  {
    Swapchain::SupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                &presentModeCount,
                                                details.presentModes.data());
    }

    return details;
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

VkSampleCountFlagBits VulkanMechanics::getMaxUsableSampleCount() {
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(mainDevice.physical, &physicalDeviceProperties);
  VkSampleCountFlags counts =
      physicalDeviceProperties.limits.framebufferColorSampleCounts &
      physicalDeviceProperties.limits.framebufferDepthSampleCounts;

  for (size_t i = VK_SAMPLE_COUNT_64_BIT; i >= VK_SAMPLE_COUNT_1_BIT; i >>= 1) {
    if (counts & i) {
      return static_cast<VkSampleCountFlagBits>(i);
    }
  }
  return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanMechanics::createLogicalDevice() {
  Log::text("{ +++ }", "Logical Device");
  Queues::FamilyIndices indices =
      findQueueFamilies(mainDevice.physical, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount =
          static_cast<uint32_t>(mainDevice.extensions.size()),
      .ppEnabledExtensionNames = mainDevice.extensions.data(),
      .pEnabledFeatures = &mainDevice.features};

  if (validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validation.validation.size());
    createInfo.ppEnabledLayerNames = validation.validation.data();
  }

  CE::vulkanResult(vkCreateDevice, mainDevice.physical, &createInfo, nullptr,
                   &mainDevice.logical);

  vkGetDeviceQueue(mainDevice.logical, indices.graphicsAndComputeFamily.value(),
                   0, &queues.graphics);
  vkGetDeviceQueue(mainDevice.logical, indices.graphicsAndComputeFamily.value(),
                   0, &queues.compute);
  vkGetDeviceQueue(mainDevice.logical, indices.presentFamily.value(), 0,
                   &queues.present);
}

VkSurfaceFormatKHR VulkanMechanics::Swapchain::pickSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  Log::text(Log::Style::charLeader, "Choose Swap Surface Format");

  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR VulkanMechanics::Swapchain::pickPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  Log::text(Log::Style::charLeader, "Choose Swap Present Mode");
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_MAILBOX_KHR;
}

VkExtent2D VulkanMechanics::Swapchain::pickExtent(
    VkSurfaceCapabilitiesKHR& capabilities) {
  Log::text(Log::Style::charLeader, "Choose Swap Extent");

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(Window::get().window, &width, &height);

    VkExtent2D actualExtent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void VulkanMechanics::SynchronizationObjects::create(VkDevice& logicalDevice) {
  Log::text("{ ||| }", "Sync Objects");

  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  graphicsInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CE::vulkanResult(vkCreateSemaphore, logicalDevice, &semaphoreInfo, nullptr,
                     &imageAvailableSemaphores[i]);
    CE::vulkanResult(vkCreateSemaphore, logicalDevice, &semaphoreInfo, nullptr,
                     &renderFinishedSemaphores[i]);
    CE::vulkanResult(vkCreateFence, logicalDevice, &fenceInfo, nullptr,
                     &graphicsInFlightFences[i]);
    CE::vulkanResult(vkCreateSemaphore, logicalDevice, &semaphoreInfo, nullptr,
                     &computeFinishedSemaphores[i]);
    CE::vulkanResult(vkCreateFence, logicalDevice, &fenceInfo, nullptr,
                     &computeInFlightFences[i]);
  }
}

bool VulkanMechanics::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
  Log::text(Log::Style::charLeader, "Is Device Suitable");

  Queues::FamilyIndices indices = findQueueFamilies(physicalDevice, surface);

  bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

  bool swapchainAdequate = false;
  if (extensionsSupported) {
    Swapchain::SupportDetails swapchainSupport =
        swapchain.checkSupport(physicalDevice, surface);
    swapchainAdequate = !swapchainSupport.formats.empty() &&
                        !swapchainSupport.presentModes.empty();
  }

  // VkPhysicalDeviceFeatures supportedFeatures;
  // vkGetPhysicalDeviceFeatures(mainDevice.physical, &supportedFeatures);
  //&& supportedFeatures.samplerAnisotropy

  return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

void VulkanMechanics::Swapchain::create(Device& device, VkSurfaceKHR& surface) {
  Log::text("{ <-> }", "Swap Chain");
  Swapchain::SupportDetails swapchainSupport =
      checkSupport(device.physical, surface);

  VkSurfaceFormatKHR surfaceFormat =
      pickSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode = pickPresentMode(swapchainSupport.presentModes);
  VkExtent2D extent = pickExtent(swapchainSupport.capabilities);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount;
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
      .preTransform = swapchainSupport.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE};

  Queues::FamilyIndices indices = findQueueFamilies(device.physical, surface);
  std::vector<uint32_t> queueFamilyIndices{
      indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsAndComputeFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndices.size());
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  CE::vulkanResult(vkCreateSwapchainKHR, device.logical, &createInfo, nullptr,
                   &swapchain);

  vkGetSwapchainImagesKHR(device.logical, swapchain, &imageCount, nullptr);

  images.resize(imageCount);
  imageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  std::vector<VkImage> swapchainImages(MAX_FRAMES_IN_FLIGHT);
  vkGetSwapchainImagesKHR(device.logical, swapchain, &imageCount,
                          swapchainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    images[i].image = swapchainImages[i];
    images[i].info.format = imageFormat;
    images[i].createView(VK_IMAGE_ASPECT_COLOR_BIT);
  };
}

void VulkanMechanics::createCommandPool(VkCommandPool* commandPool) {
  Log::text("{ cmd }", "Command Pool");

  VulkanMechanics::Queues::FamilyIndices queueFamilyIndices =
      findQueueFamilies(mainDevice.physical, surface);

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value()};

  CE::vulkanResult(vkCreateCommandPool, mainDevice.logical, &poolInfo, nullptr,
                   commandPool);
}

void VulkanMechanics::recreateswapchain(Pipelines& _pipelines,
                                        Resources& _resources) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(Window::get().window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Window::get().window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(mainDevice.logical);

  swapchain.destroy(mainDevice.logical);
  swapchain.create(mainDevice, surface);

  _resources.msaaImage.createColorResources(swapchain.swapChainExtent,
                                            swapchain.imageFormat,
                                            _resources.msaaImage.info.samples);
  _resources.depthImage.createDepthResources(swapchain.swapChainExtent,
                                             CE::Image::findDepthFormat(),
                                             _resources.msaaImage.info.samples);

  _resources.createFramebuffers(_pipelines);
  _resources.createDescriptorSets();

  uint32_t reset = 1;
  syncObjects.currentFrame = reset;
}

std::vector<const char*> VulkanMechanics::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);

  if (validation.enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void VulkanMechanics::Swapchain::destroy(VkDevice& logicalDevice) {
  if (logicalDevice != VK_NULL_HANDLE) {
    Log::text("{ <-> }", "Destroy Swapchain");

    for (size_t i = 0; i < framebuffers.size(); i++) {
      vkDestroyFramebuffer(logicalDevice, framebuffers[i], nullptr);
    }
    for (size_t i = 0; i < images.size(); i++) {
      vkDestroyImageView(logicalDevice, images[i].view, nullptr);
    }
    vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
  }
}

void VulkanMechanics::SynchronizationObjects::destroy(VkDevice& logicalDevice) {
  if (logicalDevice != VK_NULL_HANDLE) {
    Log::text("{ ||| }", "Destroy Synchronization Objects");
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(logicalDevice, computeFinishedSemaphores[i], nullptr);
      vkDestroyFence(logicalDevice, graphicsInFlightFences[i], nullptr);
      vkDestroyFence(logicalDevice, computeInFlightFences[i], nullptr);
    };
  }
}
