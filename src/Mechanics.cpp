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
      mainDevice{VK_NULL_HANDLE, VK_NULL_HANDLE},
      queues{VK_NULL_HANDLE,
             VK_NULL_HANDLE,
             VK_NULL_HANDLE,
             {std::nullopt, std::nullopt}},
      swapChain{VK_NULL_HANDLE, {}, VK_FORMAT_UNDEFINED, {}, {}, {}} {
  Log::text("{ Vk. }", "constructing Vulkan Mechanics");
}

VulkanMechanics::~VulkanMechanics() {
  Log::text("{ Vk. }", "destructing Vulkan Mechanics");
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(mainDevice.logical,
                       syncObjects.renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(mainDevice.logical,
                       syncObjects.imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(mainDevice.logical,
                       syncObjects.computeFinishedSemaphores[i], nullptr);
    vkDestroyFence(mainDevice.logical, syncObjects.inFlightFences[i], nullptr);
    vkDestroyFence(mainDevice.logical, syncObjects.computeInFlightFences[i],
                   nullptr);
  }
  vkDestroyDevice(mainDevice.logical, nullptr);
  mainDevice.logical = VK_NULL_HANDLE;

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

  pickPhysicalDevice(_resources.msaaImage);
  createLogicalDevice();

  createSwapChain();

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
    Resources::MultiSamplingImage& msaaImage) {
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
      msaaImage.sampleCount = getMaxUsableSampleCount();
      break;
    }
  }
  if (mainDevice.physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

VulkanMechanics::Queues::FamilyIndices VulkanMechanics::findQueueFamilies(
    VkPhysicalDevice physicalDevice) {
  Log::text(Log::Style::charLeader, "Find Queue Families");

  VulkanMechanics::Queues::FamilyIndices indices;

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

VulkanMechanics::SwapChain::SupportDetails
VulkanMechanics::querySwapChainSupport(VkPhysicalDevice physicalDevice) {
  Log::text(Log::Style::charLeader, "Query Swap Chain Support");
  {
    SwapChain::SupportDetails details;

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
  Queues::FamilyIndices indices = findQueueFamilies(mainDevice.physical);

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

VkSurfaceFormatKHR VulkanMechanics::chooseSwapSurfaceFormat(
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

VkPresentModeKHR VulkanMechanics::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  Log::text(Log::Style::charLeader, "Choose Swap Present Mode");
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_MAILBOX_KHR;
}

VkExtent2D VulkanMechanics::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) {
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

void VulkanMechanics::createSyncObjects() {
  Log::text("{ ||| }", "Sync Objects");

  syncObjects.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  syncObjects.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  syncObjects.computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  syncObjects.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  syncObjects.computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CE::vulkanResult(vkCreateSemaphore, mainDevice.logical, &semaphoreInfo,
                     nullptr, &syncObjects.imageAvailableSemaphores[i]);
    CE::vulkanResult(vkCreateSemaphore, mainDevice.logical, &semaphoreInfo,
                     nullptr, &syncObjects.renderFinishedSemaphores[i]);
    CE::vulkanResult(vkCreateFence, mainDevice.logical, &fenceInfo, nullptr,
                     &syncObjects.inFlightFences[i]);
    CE::vulkanResult(vkCreateSemaphore, mainDevice.logical, &semaphoreInfo,
                     nullptr, &syncObjects.computeFinishedSemaphores[i]);
    CE::vulkanResult(vkCreateFence, mainDevice.logical, &fenceInfo, nullptr,
                     &syncObjects.computeInFlightFences[i]);
  }
}

void VulkanMechanics::cleanupSwapChain(Resources& _resources) {
  for (auto& framebuffer : swapChain.framebuffers) {
    vkDestroyFramebuffer(mainDevice.logical, framebuffer, nullptr);
  }

  for (auto& image : swapChain.images) {
    vkDestroyImageView(mainDevice.logical, image.view, nullptr);
  }

  vkDestroySwapchainKHR(mainDevice.logical, swapChain.swapChain, nullptr);
}

bool VulkanMechanics::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
  Log::text(Log::Style::charLeader, "Is Device Suitable");

  Queues::FamilyIndices indices = findQueueFamilies(physicalDevice);

  bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChain::SupportDetails swapChainSupport =
        querySwapChainSupport(physicalDevice);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  // VkPhysicalDeviceFeatures supportedFeatures;
  // vkGetPhysicalDeviceFeatures(mainDevice.physical, &supportedFeatures);
  //&& supportedFeatures.samplerAnisotropy

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void VulkanMechanics::createSwapChain() {
  Log::text("{ <-> }", "Swap Chain");
  SwapChain::SupportDetails swapChainSupport =
      querySwapChainSupport(mainDevice.physical);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
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
      .preTransform = swapChainSupport.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE};

  Queues::FamilyIndices indices = findQueueFamilies(mainDevice.physical);
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

  CE::vulkanResult(vkCreateSwapchainKHR, mainDevice.logical, &createInfo,
                   nullptr, &swapChain.swapChain);

  vkGetSwapchainImagesKHR(mainDevice.logical, swapChain.swapChain, &imageCount,
                          nullptr);

  swapChain.images.resize(imageCount);
  swapChain.imageFormat = surfaceFormat.format;
  swapChain.extent = extent;

  std::vector<VkImage> swapChainImages(MAX_FRAMES_IN_FLIGHT);
  vkGetSwapchainImagesKHR(mainDevice.logical, swapChain.swapChain, &imageCount,
                          swapChainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    swapChain.images[i].image = swapChainImages[i];
    swapChain.images[i].createView(swapChain.imageFormat,
                                   VK_IMAGE_ASPECT_COLOR_BIT);
  };
}

void VulkanMechanics::createCommandPool(VkCommandPool* commandPool) {
  Log::text("{ cmd }", "Command Pool");

  VulkanMechanics::Queues::FamilyIndices queueFamilyIndices =
      findQueueFamilies(mainDevice.physical);

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value()};

  CE::vulkanResult(vkCreateCommandPool, mainDevice.logical, &poolInfo, nullptr,
                   commandPool);
}

void VulkanMechanics::recreateSwapChain(Pipelines& _pipelines,
                                        Resources& _resources) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(Window::get().window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Window::get().window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(mainDevice.logical);

  cleanupSwapChain(_resources);
  createSwapChain();

  _resources.createDepthResources();

  _resources.msaaImage.recreate();
  CE::Image::create(swapChain.extent.width, swapChain.extent.height,
                    _resources.msaaImage.sampleCount, swapChain.imageFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    _resources.msaaImage.image, _resources.msaaImage.memory);
  _resources.msaaImage.createView(swapChain.imageFormat,
                                  VK_IMAGE_ASPECT_COLOR_BIT);

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
