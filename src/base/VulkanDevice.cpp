#include "VulkanDevice.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

#include "../core/Log.h"

#include <set>
#include <stdexcept>

CE::Device *CE::Device::base_device = nullptr;
std::vector<VkDevice> CE::Device::destroyed_devices;

void CE::Device::create_logical_device(const InitializeVulkan &init_vulkan,
                     Queues &queues) {
  Log::text("{ +++ }", "Logical Device");

  const std::vector<VkDeviceQueueCreateInfo> queue_create_infos =
      fill_queue_create_infos(queues);
  VkDeviceCreateInfo create_info = get_device_create_info(queue_create_infos);
  set_validation_layers(init_vulkan, create_info);

    CE::vulkan_result(
    vkCreateDevice, this->physical_device, &create_info, nullptr, &this->logical_device);
  Log::text(
    "{ GPU }", Log::function_name(__func__), "Logical Device created", this->logical_device);
  vkGetDeviceQueue(this->logical_device,
           queues.family_indices.graphics_and_compute_family.value(),
                   0,
           &queues.graphics_queue);
  vkGetDeviceQueue(this->logical_device,
           queues.family_indices.graphics_and_compute_family.value(),
                   0,
           &queues.compute_queue);
  vkGetDeviceQueue(
    this->logical_device, queues.family_indices.present_family.value(), 0,
    &queues.present_queue);

  Log::text(Log::Style::charLeader,
            "graphics/compute queue family",
      queues.family_indices.graphics_and_compute_family.value());
  Log::text(Log::Style::charLeader,
            "present queue family",
      queues.family_indices.present_family.value());
  Log::text(Log::Style::charLeader,
            "queue handles",
      queues.graphics_queue,
      queues.compute_queue,
      queues.present_queue);
}

void CE::Device::pick_physical_device(const InitializeVulkan &init_vulkan,
                    Queues &queues,
                    Swapchain &swapchain) {
  Log::text("{ ### }", "Physical Device");
  std::vector<VkPhysicalDevice> devices = fill_devices(init_vulkan);
  Log::text("{ GPU }",
            Log::function_name(__func__),
            "Enumerated Vulkan physical devices",
            devices.size());

  for (const auto &device : devices) {
    if (is_device_suitable(device, queues, init_vulkan, swapchain)) {
      this->physical_device = device;
      get_max_usable_sample_count();
      Log::text(Log::Style::charLeader,
                Log::getSampleCountString(this->max_usable_sample_count));
      Log::text(
          Log::Style::charLeader, "selected physical device", this->physical_device);
      break;
    }
  }
  if (this->physical_device == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

std::vector<VkDeviceQueueCreateInfo>
CE::Device::fill_queue_create_infos(const Queues &queues) const {
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
  const std::set<uint32_t> unique_queue_families = {
      queues.family_indices.graphics_and_compute_family.value(),
      queues.family_indices.present_family.value()};

  const float queue_priority = 1.0f;
  for (uint_fast8_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority};
    queue_create_infos.push_back(queue_create_info);
  }
  return queue_create_infos;
}

VkDeviceCreateInfo CE::Device::get_device_create_info(
    const std::vector<VkDeviceQueueCreateInfo> &queue_create_infos) const {
  VkDeviceCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(this->extensions_.size()),
      .ppEnabledExtensionNames = this->extensions_.data(),
      .pEnabledFeatures = &this->features};
  return create_info;
}

void CE::Device::set_validation_layers(const InitializeVulkan &init_vulkan,
                                       VkDeviceCreateInfo &create_info) {
  if (init_vulkan.validation.enable_validation_layers) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(init_vulkan.validation.validation.size());
    create_info.ppEnabledLayerNames = init_vulkan.validation.validation.data();
  }
}

std::vector<VkPhysicalDevice>
CE::Device::fill_devices(const InitializeVulkan &init_vulkan) const {
  uint32_t device_count(0);
  vkEnumeratePhysicalDevices(init_vulkan.instance, &device_count, nullptr);

  if (device_count == 0) {
    throw std::runtime_error("\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(init_vulkan.instance, &device_count, devices.data());
  return devices;
}

bool CE::Device::is_device_suitable(const VkPhysicalDevice &physical_device,
                                    Queues &queues,
                                    const InitializeVulkan &init_vulkan,
                                    Swapchain &swapchain) {
  Log::text(Log::Style::charLeader, "Is Device Suitable");

  queues.family_indices =
      queues.find_queue_families(physical_device, init_vulkan.surface);
  const bool extensions_supported = check_device_extension_support(physical_device);

  bool swapchain_adequate = false;
  if (extensions_supported) {
    Swapchain::SupportDetails swapchain_support =
      swapchain.check_support(physical_device, init_vulkan.surface);
    swapchain_adequate =
      !swapchain_support.formats.empty() &&
      !swapchain_support.present_modes.empty();
  }
  Log::text(Log::Style::charLeader,
            "queueComplete",
            queues.family_indices.is_complete(),
            "extensions",
            extensions_supported,
            "swapchainAdequate",
            swapchain_adequate);
  return queues.family_indices.is_complete() && extensions_supported &&
         swapchain_adequate;
}

void CE::Device::get_max_usable_sample_count() {
  vkGetPhysicalDeviceProperties(this->physical_device, &this->properties_);
  const VkSampleCountFlags counts =
      this->properties_.limits.framebufferColorSampleCounts &
                              this->properties_.limits.framebufferDepthSampleCounts;
  for (uint_fast8_t i = VK_SAMPLE_COUNT_64_BIT; i >= VK_SAMPLE_COUNT_1_BIT; i >>= 1) {
    if (counts & i) {
      this->max_usable_sample_count = static_cast<VkSampleCountFlagBits>(i);
      return;
    } else {
      this->max_usable_sample_count = VK_SAMPLE_COUNT_1_BIT;
    }
  }
  return;
}

bool CE::Device::check_device_extension_support(
    const VkPhysicalDevice &physical_device) const {
  Log::text(Log::Style::charLeader, "Check Device Extension Support");
  uint32_t extension_count(0);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, available_extensions.data());
  Log::text(Log::Style::charLeader,
            "available extensions",
            extension_count,
            "required",
            this->extensions_.size());

  std::set<std::string> required_extensions(this->extensions_.begin(),
                                            this->extensions_.end());

  for (const auto &extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }
  if (!required_extensions.empty()) {
    Log::text("{ GPU }",
              Log::function_name(__func__),
              "missing required device extensions",
              required_extensions.size());
  }
  return required_extensions.empty();
}

void CE::Device::destroy_device() {
  static bool is_device_destroyed = false;
  for (const VkDevice &device : destroyed_devices) {
    if (device == this->logical_device) {
      is_device_destroyed = true;
      break;
    }
  }
  if (!is_device_destroyed) {
    Log::text(
        "{ +++ }", "Destroy Device", this->logical_device, "@", &this->logical_device);
    extensions_.clear();
    vkDestroyDevice(this->logical_device, nullptr);
    destroyed_devices.push_back(this->logical_device);
    if (Device::base_device == this) {
      Device::base_device = nullptr;
    }
    this->logical_device = VK_NULL_HANDLE;
  }
}

CE::Queues::FamilyIndices
CE::Queues::find_queue_families(const VkPhysicalDevice &physical_device,
                              const VkSurfaceKHR &surface) const {
  Log::text(Log::Style::charLeader, "Find Queue Families");

  CE::Queues::FamilyIndices indices{};
  uint32_t queue_family_count(0);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, queue_families.data());

  int i(0);
  for (const auto &queue_family : queue_families) {
    if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.graphics_and_compute_family = i;
    }
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
    if (present_support) {
      indices.present_family = i;
    }
    if (indices.is_complete()) {
      Log::text(Log::Style::charLeader,
                "selected queue families",
                "gc",
                indices.graphics_and_compute_family.value(),
                "present",
                indices.present_family.value());
      break;
    }
    i++;
  }
  return indices;
}

CE::InitializeVulkan::InitializeVulkan() {
  Log::text("{ VkI }", "constructing Initialize Vulkan");
  create_instance();
  this->validation.setup_debug_messenger(this->instance);
  create_surface(Window::get().window);
}

CE::InitializeVulkan::~InitializeVulkan() {
  Log::text("{ VkI }", "destructing Initialize Vulkan");
  if (this->validation.enable_validation_layers) {
    this->validation.destroy_debug_utils_messenger_ext(
        this->instance, this->validation.debug_messenger, nullptr);
  }
  vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
  vkDestroyInstance(this->instance, nullptr);
}

void CE::InitializeVulkan::create_instance() {
  Log::text("{ VkI }", "Vulkan Instance");
    if (this->validation.enable_validation_layers &&
      !this->validation.check_validation_layer_support()) {
    throw std::runtime_error("\n!ERROR! validation layers requested, but not available!");
  }

  const VkApplicationInfo app_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             .pApplicationName = Window::get().display.title,
                             .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                             .pEngineName = "CAPITAL Engine",
                             .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                             .apiVersion = VK_API_VERSION_1_3};
  Log::text(Log::Style::charLeader,
            app_info.pApplicationName,
            app_info.applicationVersion,
            "-",
            app_info.pEngineName,
            app_info.engineVersion,
            "-",
            "Vulkan",
            1.3);

  const std::vector<const char *> extensions = get_required_extensions();

  VkInstanceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                  .pNext = nullptr,
                                  .pApplicationInfo = &app_info,
                                  .enabledLayerCount = 0,
                                  .enabledExtensionCount =
                                      static_cast<uint32_t>(extensions.size()),
                                  .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
  if (this->validation.enable_validation_layers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(this->validation.validation.size());
    createInfo.ppEnabledLayerNames = this->validation.validation.data();

    this->validation.populate_debug_messenger_create_info(debug_create_info);
    createInfo.pNext = &debug_create_info;
  }
    CE::vulkan_result(vkCreateInstance, &createInfo, nullptr, &this->instance);
}

void CE::InitializeVulkan::create_surface(GLFWwindow *window) {
  Log::text("{ [ ] }", "Surface");
    CE::vulkan_result(
      glfwCreateWindowSurface, this->instance, window, nullptr, &this->surface);
}

std::vector<const char *> CE::InitializeVulkan::get_required_extensions() const {
  uint32_t glfw_extension_count(0);
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  std::vector<const char *> extensions(glfw_extensions,
                                       glfw_extensions + glfw_extension_count);
  if (this->validation.enable_validation_layers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  return extensions;
}
