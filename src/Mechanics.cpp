#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Mechanics.h"

#include <algorithm>
#include <set>
#include <stdexcept>

VulkanMechanics::VulkanMechanics()
    : surface(VK_NULL_HANDLE),
      instance(VK_NULL_HANDLE),
      mainDevice{VK_NULL_HANDLE, VK_NULL_HANDLE},
      queues{VK_NULL_HANDLE,
             VK_NULL_HANDLE,
             VK_NULL_HANDLE,
             {std::nullopt, std::nullopt}},
      swapChain{VK_NULL_HANDLE, {}, VK_FORMAT_UNDEFINED, {}, {0, 0}, {}, {}} {
  _log.console("{ VkM }", "constructing Vulkan Mechanics");
}

VulkanMechanics::~VulkanMechanics() {
  _log.console("{ VkM }", "destructing Vulkan Mechanics");
}

void VulkanMechanics::setupVulkan() {
  _log.console("{ VK. }", "setting up Vulkan");
  compileShaders();
  createInstance();
  _validation.setupDebugMessenger(instance);
  createSurface();

  pickPhysicalDevice();
  createLogicalDevice();

  createSwapChain();
  createSwapChainImageViews();

  createCommandPool();
}

void VulkanMechanics::createInstance() {
  _log.console("{ VkI }", "creating Vulkan Instance");
  if (_validation.enableValidationLayers &&
      !_validation.checkValidationLayerSupport()) {
    throw std::runtime_error(
        "\n!ERROR! validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "CAPITAL",
                            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                            .pEngineName = _control.display.title,
                            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                            .apiVersion = VK_API_VERSION_1_3};
  _log.console(
      _log.style.charLeader, "Application name:", appInfo.pApplicationName,
      "\n", _log.style.indentSize, _log.style.charLeader,
      "Application Version:", appInfo.applicationVersion, "\n",
      _log.style.indentSize, _log.style.charLeader,
      "Engine Name Version:", appInfo.pEngineName, "\n", _log.style.indentSize,
      _log.style.charLeader, "Engine Version:", appInfo.engineVersion, "\n",
      _log.style.indentSize, _log.style.charLeader, "API Version:", 1.3);

  auto extensions = getRequiredExtensions();

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (_validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(_validation.validation.size());
    createInfo.ppEnabledLayerNames = _validation.validation.data();

    _validation.populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }

  result(vkCreateInstance, &createInfo, nullptr, &instance);
}

void VulkanMechanics::createSurface() {
  _log.console("{ [ ] }", "creating Surface");
  result(glfwCreateWindowSurface, instance, _window.window, nullptr, &surface);
}

void VulkanMechanics::pickPhysicalDevice() {
  _log.console("{ ### }", "picking Physical Device");
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
      _pipelines.graphics.msaa.samples = getMaxUsableSampleCount();
      break;
    }
  }
  if (mainDevice.physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

VulkanMechanics::Queues::FamilyIndices VulkanMechanics::findQueueFamilies(
    VkPhysicalDevice physicalDevice) {
  _log.console(_log.style.charLeader, "finding Queue Families");

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
  _log.console(_log.style.charLeader, "querying Swap Chain Support");
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
  _log.console(_log.style.charLeader, "checking Device Extension Support");
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
  _log.console("{ +++ }", "creating Logical Device");
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

  VkPhysicalDeviceFeatures deviceFeatures{//.tessellationShader = VK_TRUE,
                                          .sampleRateShading = VK_TRUE,
                                          .depthClamp = VK_TRUE,
                                          .depthBiasClamp = VK_TRUE,
                                          .samplerAnisotropy = VK_TRUE,
                                          .shaderInt64 = VK_TRUE};

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount =
          static_cast<uint32_t>(mainDevice.extensions.size()),
      .ppEnabledExtensionNames = mainDevice.extensions.data(),
      .pEnabledFeatures = &deviceFeatures};

  if (_validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(_validation.validation.size());
    createInfo.ppEnabledLayerNames = _validation.validation.data();
  }

  result(vkCreateDevice, mainDevice.physical, &createInfo, nullptr,
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
  _log.console(_log.style.charLeader, "choosing Swap Surface Format");

  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR VulkanMechanics::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  _log.console(_log.style.charLeader, "choosing Swap Present Mode");
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_MAILBOX_KHR;
}

VkExtent2D VulkanMechanics::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) {
  _log.console(_log.style.charLeader, "choosing Swap Extent");

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(_window.window, &width, &height);

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
  _log.console("{ ||| }", "creating Sync Objects");

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
    result(vkCreateSemaphore, mainDevice.logical, &semaphoreInfo, nullptr,
           &syncObjects.imageAvailableSemaphores[i]);

    result(vkCreateSemaphore, mainDevice.logical, &semaphoreInfo, nullptr,
           &syncObjects.renderFinishedSemaphores[i]);

    result(vkCreateFence, mainDevice.logical, &fenceInfo, nullptr,
           &syncObjects.inFlightFences[i]);

    result(vkCreateSemaphore, mainDevice.logical, &semaphoreInfo, nullptr,
           &syncObjects.computeFinishedSemaphores[i]);

    result(vkCreateFence, mainDevice.logical, &fenceInfo, nullptr,
           &syncObjects.computeInFlightFences[i]);
  }
}

void VulkanMechanics::cleanupSwapChain() {
  vkDestroyImageView(mainDevice.logical, _pipelines.graphics.depth.imageView,
                     nullptr);
  vkDestroyImage(mainDevice.logical, _pipelines.graphics.depth.image, nullptr);
  vkFreeMemory(mainDevice.logical, _pipelines.graphics.depth.imageMemory,
               nullptr);

  vkDestroyImageView(mainDevice.logical,
                     _pipelines.graphics.msaa.colorImageView, nullptr);
  vkDestroyImage(mainDevice.logical, _pipelines.graphics.msaa.colorImage,
                 nullptr);
  vkFreeMemory(mainDevice.logical, _pipelines.graphics.msaa.colorImageMemory,
               nullptr);

  for (auto framebuffer : swapChain.framebuffers) {
    vkDestroyFramebuffer(mainDevice.logical, framebuffer, nullptr);
  }

  for (auto imageView : swapChain.imageViews) {
    vkDestroyImageView(mainDevice.logical, imageView, nullptr);
  }

  vkDestroySwapchainKHR(mainDevice.logical, swapChain.swapChain, nullptr);
}

bool VulkanMechanics::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
  _log.console(_log.style.charLeader,
               "checking if Physical Device is suitable");

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
  _log.console("{ <-> }", "creating Swap Chain");
  SwapChain::SupportDetails swapChainSupport =
      querySwapChainSupport(mainDevice.physical);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
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
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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

  result(vkCreateSwapchainKHR, mainDevice.logical, &createInfo, nullptr,
         &swapChain.swapChain);

  vkGetSwapchainImagesKHR(mainDevice.logical, swapChain.swapChain, &imageCount,
                          nullptr);
  swapChain.images.resize(imageCount);
  vkGetSwapchainImagesKHR(mainDevice.logical, swapChain.swapChain, &imageCount,
                          swapChain.images.data());

  swapChain.imageFormat = surfaceFormat.format;
  swapChain.extent = extent;
}

void VulkanMechanics::createSwapChainImageViews() {
  _log.console("{ IMG }", "Creating", swapChain.images.size(), "Image Views");

  swapChain.imageViews.resize(swapChain.images.size());

  for (size_t i = 0; i < swapChain.images.size(); i++) {
    swapChain.imageViews[i] = _resources.createImageView(
        swapChain.images[i], swapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

void VulkanMechanics::createCommandPool() {
  _log.console("{ CMD }", "creating Command Pool");

  VulkanMechanics::Queues::FamilyIndices queueFamilyIndices =
      findQueueFamilies(mainDevice.physical);

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value()};

  result(vkCreateCommandPool, mainDevice.logical, &poolInfo, nullptr,
         &_resources.buffers.command.pool);
}

void VulkanMechanics::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(_window.window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(_window.window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(mainDevice.logical);

  cleanupSwapChain();

  createSwapChain();
  createSwapChainImageViews();
  _pipelines.createDepthResources();
  _pipelines.createColorResources();
  _resources.createFramebuffers();
}

void VulkanMechanics::compileShaders() {
  _log.console("{ SHA }", "compiling shaders");
#ifdef _WIN32
  // auto err = std::system("cmd /C \"..\\shaders\\compile_shaders.bat >
  // NUL\"");
  auto err = std::system("..\\shaders\\compile_shaders.bat");

#else
  // Linux-specific code
  auto err = std::system("./shaders/compile_shaders.sh");
#endif
}

std::vector<const char*> VulkanMechanics::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);

  if (_validation.enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}
