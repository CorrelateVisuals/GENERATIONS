#include "VulkanDevice.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

#include "../Log.h"

#include <set>
#include <stdexcept>

CE::Device* CE::Device::baseDevice = nullptr;
std::vector<VkDevice> CE::Device::destroyedDevices;

void CE::Device::createLogicalDevice(const InitializeVulkan& initVulkan,
                                     Queues& queues) {
  Log::text("{ +++ }", "Logical Device");

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos =
      fillQueueCreateInfos(queues);
  VkDeviceCreateInfo createInfo = getDeviceCreateInfo(queueCreateInfos);
  setValidationLayers(initVulkan, createInfo);

  CE::VULKAN_RESULT(vkCreateDevice, this->physical, &createInfo, nullptr,
                    &this->logical);
  vkGetDeviceQueue(this->logical,
                   queues.familyIndices.graphicsAndComputeFamily.value(), 0,
                   &queues.graphics);
  vkGetDeviceQueue(this->logical,
                   queues.familyIndices.graphicsAndComputeFamily.value(), 0,
                   &queues.compute);
  vkGetDeviceQueue(this->logical, queues.familyIndices.presentFamily.value(),
                   0, &queues.present);
}

void CE::Device::pickPhysicalDevice(const InitializeVulkan& initVulkan,
                                    Queues& queues,
                                    Swapchain& swapchain) {
  Log::text("{ ### }", "Physical Device");
  std::vector<VkPhysicalDevice> devices = fillDevices(initVulkan);

  for (const auto& device : devices) {
    if (isDeviceSuitable(device, queues, initVulkan, swapchain)) {
      this->physical = device;
      getMaxUsableSampleCount();
      Log::text(Log::Style::charLeader,
                Log::getSampleCountString(this->maxUsableSampleCount));
      break;
    }
  }
  if (this->physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

std::vector<VkDeviceQueueCreateInfo> CE::Device::fillQueueCreateInfos(
    const Queues& queues) const {
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> uniqueQueueFamilies = {
      queues.familyIndices.graphicsAndComputeFamily.value(),
      queues.familyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint_fast8_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }
  return queueCreateInfos;
}

VkDeviceCreateInfo CE::Device::getDeviceCreateInfo(
    const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos) const {
  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(this->extensions.size()),
      .ppEnabledExtensionNames = this->extensions.data(),
      .pEnabledFeatures = &this->features};
  return createInfo;
}

void CE::Device::setValidationLayers(const InitializeVulkan& initVulkan,
                                     VkDeviceCreateInfo& createInfo) {
  if (initVulkan.validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(initVulkan.validation.validation.size());
    createInfo.ppEnabledLayerNames = initVulkan.validation.validation.data();
  }
}

std::vector<VkPhysicalDevice> CE::Device::fillDevices(
    const InitializeVulkan& initVulkan) const {
  uint32_t deviceCount(0);
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error(
        "\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, devices.data());
  return devices;
}

bool CE::Device::isDeviceSuitable(const VkPhysicalDevice& physical,
                                  Queues& queues,
                                  const InitializeVulkan& initVulkan,
                                  Swapchain& swapchain) {
  Log::text(Log::Style::charLeader, "Is Device Suitable");

  queues.familyIndices = queues.findQueueFamilies(physical, initVulkan.surface);
  bool extensionsSupported = checkDeviceExtensionSupport(physical);

  bool swapchainAdequate = false;
  if (extensionsSupported) {
    Swapchain::SupportDetails swapchainSupport =
        swapchain.checkSupport(physical, initVulkan.surface);
    swapchainAdequate = !swapchainSupport.formats.empty() &&
                        !swapchainSupport.presentModes.empty();
  }
  return queues.familyIndices.isComplete() && extensionsSupported &&
         swapchainAdequate;
}

void CE::Device::getMaxUsableSampleCount() {
  vkGetPhysicalDeviceProperties(this->physical, &this->properties);
  VkSampleCountFlags counts =
      this->properties.limits.framebufferColorSampleCounts &
      this->properties.limits.framebufferDepthSampleCounts;
  for (uint_fast8_t i = VK_SAMPLE_COUNT_64_BIT; i >= VK_SAMPLE_COUNT_1_BIT;
       i >>= 1) {
    if (counts & i) {
      this->maxUsableSampleCount = static_cast<VkSampleCountFlagBits>(i);
      return;
    } else {
      this->maxUsableSampleCount = VK_SAMPLE_COUNT_1_BIT;
    }
  }
  return;
}

bool CE::Device::checkDeviceExtensionSupport(
    const VkPhysicalDevice& physical) const {
  Log::text(Log::Style::charLeader, "Check Device Extension Support");
  uint32_t extensionCount(0);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(this->extensions.begin(),
                                           this->extensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions.empty();
}

void CE::Device::destroyDevice() {
  static bool isDeviceDestroyed = false;
  for (const VkDevice& device : destroyedDevices) {
    if (device == this->logical) {
      isDeviceDestroyed = true;
      break;
    }
  }
  if (!isDeviceDestroyed) {
    Log::text("{ +++ }", "Destroy Device", this->logical, "@", &this->logical);
    extensions.clear();
    vkDestroyDevice(this->logical, nullptr);
    destroyedDevices.push_back(this->logical);
    if (Device::baseDevice == this) {
      Device::baseDevice = nullptr;
    }
    this->logical = VK_NULL_HANDLE;
  }
}

CE::Queues::FamilyIndices CE::Queues::findQueueFamilies(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface) const {
  Log::text(Log::Style::charLeader, "Find Queue Families");

  CE::Queues::FamilyIndices indices{};
  uint32_t queueFamilyCount(0);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int i(0);
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

CE::InitializeVulkan::InitializeVulkan() {
  Log::text("{ VkI }", "constructing Initialize Vulkan");
  createInstance();
  this->validation.setupDebugMessenger(this->instance);
  createSurface(Window::get().window);
}

CE::InitializeVulkan::~InitializeVulkan() {
  Log::text("{ VkI }", "destructing Initialize Vulkan");
  if (this->validation.enableValidationLayers) {
    this->validation.DestroyDebugUtilsMessengerEXT(
        this->instance, this->validation.debugMessenger, nullptr);
  }
  vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
  vkDestroyInstance(this->instance, nullptr);
}

void CE::InitializeVulkan::createInstance() {
  Log::text("{ VkI }", "Vulkan Instance");
  if (this->validation.enableValidationLayers &&
      !this->validation.checkValidationLayerSupport()) {
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

  std::vector<const char*> extensions = getRequiredExtensions();

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (this->validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(this->validation.validation.size());
    createInfo.ppEnabledLayerNames = this->validation.validation.data();

    this->validation.populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }
  CE::VULKAN_RESULT(vkCreateInstance, &createInfo, nullptr, &this->instance);
}

void CE::InitializeVulkan::createSurface(GLFWwindow* window) {
  Log::text("{ [ ] }", "Surface");
  CE::VULKAN_RESULT(glfwCreateWindowSurface, this->instance, window, nullptr,
                    &this->surface);
}

std::vector<const char*> CE::InitializeVulkan::getRequiredExtensions() const {
  uint32_t glfwExtensionCount(0);
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);
  if (this->validation.enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  return extensions;
}
