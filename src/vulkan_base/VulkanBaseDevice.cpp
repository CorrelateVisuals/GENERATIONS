#include "VulkanBaseDevice.h"
#include "VulkanBaseSync.h"
#include "VulkanBaseUtils.h"

#include "engine/Log.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <set>
#include <stdexcept>

CE::BaseDevice *CE::BaseDevice::base_device = nullptr;
std::vector<VkDevice> CE::BaseDevice::destroyed_devices;

namespace {

struct GpuLogSettings {
  bool enabled = true;
  bool startup = true;
  bool periodic = false;
  bool detailed = false;
  uint32_t frequency_ms = 5000;
};

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool parse_bool_env(const char *name, const bool default_value) {
  const char *raw = std::getenv(name);
  if (!raw) {
    return default_value;
  }

  const std::string value = lowercase(raw);
  if (value == "1" || value == "true" || value == "on" || value == "yes") {
    return true;
  }
  if (value == "0" || value == "false" || value == "off" || value == "no") {
    return false;
  }
  return default_value;
}

uint32_t parse_uint_env(const char *name, const uint32_t default_value) {
  const char *raw = std::getenv(name);
  if (!raw) {
    return default_value;
  }

  char *end = nullptr;
  const unsigned long parsed = std::strtoul(raw, &end, 10);
  if (end == raw || *end != '\0' || parsed > std::numeric_limits<uint32_t>::max()) {
    return default_value;
  }
  return static_cast<uint32_t>(parsed);
}

const GpuLogSettings &gpu_log_settings() {
  static const GpuLogSettings settings = [] {
    GpuLogSettings configured{};
    configured.enabled = parse_bool_env("CE_GPU_LOG", true);
    configured.startup = parse_bool_env("CE_GPU_LOG_STARTUP", true);
    configured.periodic = parse_bool_env("CE_GPU_LOG_PERIODIC", false);
    configured.frequency_ms =
        std::max<uint32_t>(250, parse_uint_env("CE_GPU_LOG_FREQ_MS", 5000));

    const char *detail_raw = std::getenv("CE_GPU_LOG_DETAILS");
    if (detail_raw) {
      const std::string detail = lowercase(detail_raw);
      configured.detailed =
          (detail == "1" || detail == "true" || detail == "on" || detail == "full" ||
           detail == "detailed");
    }

    if (Log::gpu_trace_enabled()) {
      configured.detailed = true;
    }
    return configured;
  }();

  return settings;
}

const char *device_type_name(const VkPhysicalDeviceType type) {
  switch (type) {
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    return "Discrete";
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    return "Integrated";
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    return "Virtual";
  case VK_PHYSICAL_DEVICE_TYPE_CPU:
    return "CPU";
  default:
    return "Other";
  }
}

std::string format_bytes(const VkDeviceSize bytes) {
  const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << gib << " GiB";
  return stream.str();
}

std::string version_string(const uint32_t version) {
  std::ostringstream stream;
  stream << VK_VERSION_MAJOR(version) << '.' << VK_VERSION_MINOR(version) << '.'
         << VK_VERSION_PATCH(version);
  return stream.str();
}

bool query_local_memory_budget(const VkPhysicalDevice &physical_device,
                               VkDeviceSize &total_usage,
                               VkDeviceSize &total_budget,
                               const bool verbose_output) {
  VkPhysicalDeviceMemoryBudgetPropertiesEXT budget_properties{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT,
      .pNext = nullptr};
  VkPhysicalDeviceMemoryProperties2 memory_properties_2{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
      .pNext = &budget_properties};
  vkGetPhysicalDeviceMemoryProperties2(physical_device, &memory_properties_2);

  total_budget = 0;
  total_usage = 0;
  for (uint32_t heap_index = 0;
       heap_index < memory_properties_2.memoryProperties.memoryHeapCount;
       ++heap_index) {
    const VkMemoryHeap &heap = memory_properties_2.memoryProperties.memoryHeaps[heap_index];
    if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0) {
      continue;
    }

    total_budget += budget_properties.heapBudget[heap_index];
    total_usage += budget_properties.heapUsage[heap_index];

    if (verbose_output) {
      Log::text(Log::Style::char_leader,
                "heap budget",
                heap_index,
                format_bytes(budget_properties.heapUsage[heap_index]),
                "/",
                format_bytes(budget_properties.heapBudget[heap_index]));
    }
  }

  return total_budget > 0;
}

bool has_extension(const std::vector<VkExtensionProperties> &available_extensions,
                   const char *name) {
  for (const VkExtensionProperties &extension : available_extensions) {
    if (std::strcmp(extension.extensionName, name) == 0) {
      return true;
    }
  }
  return false;
}

VkDeviceSize get_local_heap_total(const VkPhysicalDeviceMemoryProperties &memory_properties) {
  VkDeviceSize total = 0;
  for (uint32_t heap_index = 0; heap_index < memory_properties.memoryHeapCount;
       ++heap_index) {
    const VkMemoryHeap &heap = memory_properties.memoryHeaps[heap_index];
    if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) {
      total += heap.size;
    }
  }
  return total;
}

void log_device_memory_heaps(const VkPhysicalDeviceMemoryProperties &memory_properties) {
  for (uint32_t heap_index = 0; heap_index < memory_properties.memoryHeapCount;
       ++heap_index) {
    const VkMemoryHeap &heap = memory_properties.memoryHeaps[heap_index];
    const bool is_device_local = (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
    Log::text("{ GPU }",
              "heap",
              heap_index,
              is_device_local ? "device_local" : "host_visible_or_shared",
              format_bytes(heap.size));
  }
}

void log_memory_budget_if_available(
    const VkPhysicalDevice &physical_device,
    const std::vector<VkExtensionProperties> &available_extensions,
    const bool verbose_output) {
  const bool supports_memory_budget =
      has_extension(available_extensions, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
  if (!supports_memory_budget) {
    return;
  }

  VkDeviceSize total_usage = 0;
  VkDeviceSize total_budget = 0;
  if (query_local_memory_budget(physical_device, total_usage, total_budget, verbose_output)) {
    const double usage_percent =
        (static_cast<double>(total_usage) / static_cast<double>(total_budget)) * 100.0;
    std::ostringstream usage_stream;
    usage_stream << std::fixed << std::setprecision(1) << usage_percent;
    Log::text("{ GPU }",
              "local memory usage",
              format_bytes(total_usage),
              "/",
              format_bytes(total_budget),
              usage_stream.str() + "%");
  }
}

void log_physical_device_snapshot(const VkPhysicalDevice &physical_device,
                                  const uint32_t device_index,
                                  const bool verbose_output) {
  VkPhysicalDeviceProperties properties{};
  VkPhysicalDeviceMemoryProperties memory_properties{};
  vkGetPhysicalDeviceProperties(physical_device, &properties);
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  uint32_t extension_count = 0;
        Log::text("{ GPU }",
      physical_device, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, available_extensions.data());

  Log::text("{ GPU }",
            "candidate",
            device_index,
            properties.deviceName,
            "type",
            device_type_name(properties.deviceType));
  Log::text(Log::Style::char_leader,
            "api",
            version_string(properties.apiVersion),
            "driver",
            version_string(properties.driverVersion),
            "vendor",
            properties.vendorID,
            "device",
            properties.deviceID);
  Log::text(Log::Style::char_leader,
            "device-local memory",
            format_bytes(get_local_heap_total(memory_properties)),
            "heaps",
            memory_properties.memoryHeapCount,
            "extensions",
            extension_count);

  log_memory_budget_if_available(physical_device, available_extensions, verbose_output);
  if (verbose_output) {
    log_device_memory_heaps(memory_properties);
  }
}

} // namespace

void CE::BaseDevice::create_logical_device(const BaseInitializeVulkan &init_vulkan,
                     BaseQueues &queues) {
  Log::text("{ +++ }", "Logical BaseDevice");

  const std::vector<VkDeviceQueueCreateInfo> queue_create_infos =
      fill_queue_create_infos(queues);
  VkDeviceCreateInfo create_info = get_device_create_info(queue_create_infos);
  set_validation_layers(init_vulkan, create_info);

    CE::vulkan_result(
    vkCreateDevice, this->physical_device, &create_info, nullptr, &this->logical_device);
  if (gpu_log_settings().enabled && gpu_log_settings().startup) {
    Log::text("{ GPU }",
              Log::function_name(__func__),
              "Logical BaseDevice created",
              this->logical_device);
  }
    vkGetDeviceQueue(this->logical_device,
       queues.indices.graphics_and_compute_family.value(),
                   0,
           &queues.graphics_queue);
  vkGetDeviceQueue(this->logical_device,
       queues.indices.graphics_and_compute_family.value(),
                   0,
           &queues.compute_queue);
  vkGetDeviceQueue(
      this->logical_device, queues.indices.present_family.value(), 0,
    &queues.present_queue);

    Log::text(Log::Style::char_leader,
            "graphics/compute queue family",
      queues.indices.graphics_and_compute_family.value());
    Log::text(Log::Style::char_leader,
            "present queue family",
      queues.indices.present_family.value());
    Log::text(Log::Style::char_leader,
            "queue handles",
      queues.graphics_queue,
      queues.compute_queue,
      queues.present_queue);
}

void CE::BaseDevice::pick_physical_device(const BaseInitializeVulkan &init_vulkan,
                    BaseQueues &queues,
                    BaseSwapchain &swapchain) {
  Log::text("{ ### }", "Physical BaseDevice");
  std::vector<VkPhysicalDevice> devices = fill_devices(init_vulkan);
  const GpuLogSettings &gpu_log = gpu_log_settings();
  const bool startup_gpu_logs = gpu_log.enabled && gpu_log.startup;
  const bool verbose_gpu_logs =
      startup_gpu_logs && (gpu_log.detailed || (Log::log_level >= Log::LOG_DETAILED));
  if (startup_gpu_logs) {
    Log::text("{ GPU }",
              "logging",
              "startup",
              gpu_log.startup ? "on" : "off",
              "periodic",
              gpu_log.periodic ? "on" : "off",
              "details",
              gpu_log.detailed ? "detailed" : "basic",
              "freq_ms",
              gpu_log.frequency_ms);
    Log::text("{ GPU }",
              Log::function_name(__func__),
              "Enumerated Vulkan physical devices",
              devices.size());
  }

  uint32_t device_index = 0;
  for (const auto &device : devices) {
    if (startup_gpu_logs) {
      log_physical_device_snapshot(device, device_index, verbose_gpu_logs);
    }
    if (is_device_suitable(device, queues, init_vulkan, swapchain)) {
      this->physical_device = device;
      VkPhysicalDeviceMemoryProperties memory_properties{};
      vkGetPhysicalDeviceMemoryProperties(this->physical_device, &memory_properties);
      this->device_local_heap_total_bytes_ = get_local_heap_total(memory_properties);

      uint32_t extension_count = 0;
      vkEnumerateDeviceExtensionProperties(
          this->physical_device, nullptr, &extension_count, nullptr);
      std::vector<VkExtensionProperties> available_extensions(extension_count);
      vkEnumerateDeviceExtensionProperties(this->physical_device,
                                           nullptr,
                                           &extension_count,
                                           available_extensions.data());
      this->memory_budget_supported_ =
          has_extension(available_extensions, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

      get_max_usable_sample_count();
      if (startup_gpu_logs) {
        Log::text(Log::Style::char_leader,
                  Log::get_sample_count_string(this->max_usable_sample_count));
        VkPhysicalDeviceProperties selected_properties{};
        vkGetPhysicalDeviceProperties(this->physical_device, &selected_properties);
        Log::text("{ GPU }",
                  "selected",
                  selected_properties.deviceName,
                  "type",
                  device_type_name(selected_properties.deviceType),
                  "handle",
                  this->physical_device);
        Log::text("{ GPU }",
                  "queue relationship",
                  "graphics+compute",
                  queues.indices.graphics_and_compute_family.value(),
                  "-> present",
                  queues.indices.present_family.value());
      }
      break;
    }
    ++device_index;
  }
  if (this->physical_device == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

void CE::BaseDevice::maybe_log_gpu_runtime_sample() {
  const GpuLogSettings &gpu_log = gpu_log_settings();
  if (!gpu_log.enabled || !gpu_log.periodic ||
      this->physical_device == VK_NULL_HANDLE) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (!this->gpu_runtime_log_initialized_) {
    this->gpu_runtime_log_initialized_ = true;
    this->last_gpu_runtime_log_ = now;
    return;
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now - this->last_gpu_runtime_log_)
                              .count();
  if (elapsed_ms < static_cast<int64_t>(gpu_log.frequency_ms)) {
    return;
  }
  this->last_gpu_runtime_log_ = now;

  if (this->memory_budget_supported_) {
    VkDeviceSize total_usage = 0;
    VkDeviceSize total_budget = 0;
    if (query_local_memory_budget(this->physical_device,
                                  total_usage,
                                  total_budget,
                                  gpu_log.detailed) &&
        total_budget > 0) {
      const double usage_percent =
          (static_cast<double>(total_usage) / static_cast<double>(total_budget)) * 100.0;
      std::ostringstream usage_stream;
      usage_stream << std::fixed << std::setprecision(1) << usage_percent;
      Log::text("{ GPU }",
                "runtime memory",
                format_bytes(total_usage),
                "/",
                format_bytes(total_budget),
                usage_stream.str() + "%");
      return;
    }
  }

  if (gpu_log.detailed || Log::gpu_trace_enabled() ||
      Log::log_level >= Log::LOG_DETAILED) {
    Log::text("{ GPU }",
              "runtime sample",
              "device-local total",
              format_bytes(this->device_local_heap_total_bytes_));
  }
}

std::vector<VkDeviceQueueCreateInfo>
CE::BaseDevice::fill_queue_create_infos(const BaseQueues &queues) const {
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
  const std::set<uint32_t> unique_queue_families = {
      queues.indices.graphics_and_compute_family.value(),
      queues.indices.present_family.value()};

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

VkDeviceCreateInfo CE::BaseDevice::get_device_create_info(
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

void CE::BaseDevice::set_validation_layers(const BaseInitializeVulkan &init_vulkan,
                                       VkDeviceCreateInfo &create_info) {
  if (init_vulkan.validation.enable_validation_layers) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(init_vulkan.validation.validation.size());
    create_info.ppEnabledLayerNames = init_vulkan.validation.validation.data();
  }
}

std::vector<VkPhysicalDevice>
CE::BaseDevice::fill_devices(const BaseInitializeVulkan &init_vulkan) const {
  uint32_t device_count(0);
  vkEnumeratePhysicalDevices(init_vulkan.instance, &device_count, nullptr);

  if (device_count == 0) {
    throw std::runtime_error("\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(init_vulkan.instance, &device_count, devices.data());
  return devices;
}

bool CE::BaseDevice::is_device_suitable(const VkPhysicalDevice &physical_device,
                                    BaseQueues &queues,
                                    const BaseInitializeVulkan &init_vulkan,
                                    BaseSwapchain &swapchain) {
  Log::text(Log::Style::char_leader, "Is BaseDevice Suitable");

  queues.indices =
      queues.find_queue_families(physical_device, init_vulkan.surface);
  const bool extensions_supported = check_device_extension_support(physical_device);

  bool swapchain_adequate = false;
  if (extensions_supported) {
    BaseSwapchain::SupportDetails swapchain_support =
      swapchain.check_support(physical_device, init_vulkan.surface);
    swapchain_adequate =
      !swapchain_support.formats.empty() &&
      !swapchain_support.present_modes.empty();
  }
  Log::text(Log::Style::char_leader,
            "queueComplete",
            queues.indices.is_complete(),
            "extensions",
            extensions_supported,
            "swapchainAdequate",
            swapchain_adequate);
  return queues.indices.is_complete() && extensions_supported &&
         swapchain_adequate;
}

void CE::BaseDevice::get_max_usable_sample_count() {
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

bool CE::BaseDevice::check_device_extension_support(
    const VkPhysicalDevice &physical_device) const {
  Log::text(Log::Style::char_leader, "Check BaseDevice Extension Support");
  uint32_t extension_count(0);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, available_extensions.data());
  Log::text(Log::Style::char_leader,
            "available extensions",
            extension_count,
            "required",
            this->extensions_.size());

  std::set<std::string> required_extensions(this->extensions_.begin(),
                                            this->extensions_.end());

  for (const auto &extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }
  if (!required_extensions.empty() && gpu_log_settings().enabled) {
    Log::text("{ GPU }",
              Log::function_name(__func__),
              "missing required device extensions",
              required_extensions.size());
  }
  return required_extensions.empty();
}

void CE::BaseDevice::destroy_device() {
  if (this->logical_device == VK_NULL_HANDLE) {
    return;
  }

  bool is_device_destroyed = false;
  for (const VkDevice &device : destroyed_devices) {
    if (device == this->logical_device) {
      is_device_destroyed = true;
      break;
    }
  }
  if (!is_device_destroyed) {
    Log::text(
        "{ +++ }", "Destroy BaseDevice", this->logical_device, "@", &this->logical_device);
    extensions_.clear();
    vkDestroyDevice(this->logical_device, nullptr);
    destroyed_devices.push_back(this->logical_device);
    if (BaseDevice::base_device == this) {
      BaseDevice::base_device = nullptr;
    }
    this->logical_device = VK_NULL_HANDLE;
  } else {
    if (BaseDevice::base_device == this) {
      BaseDevice::base_device = nullptr;
    }
    this->logical_device = VK_NULL_HANDLE;
  }
}

CE::BaseQueues::FamilyIndices
CE::BaseQueues::find_queue_families(const VkPhysicalDevice &physical_device,
                              const VkSurfaceKHR &surface) const {
  Log::text(Log::Style::char_leader, "Find Queue Families");

  CE::BaseQueues::FamilyIndices indices{};
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
      Log::text(Log::Style::char_leader,
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

CE::BaseInitializeVulkan::BaseInitializeVulkan() {
  Log::text("{ VkI }", "constructing Initialize Vulkan");
  create_instance();
  this->validation.setup_debug_messenger(this->instance);
  create_surface(Window::get().window);
}

CE::BaseInitializeVulkan::~BaseInitializeVulkan() {
  Log::text("{ VkI }", "destructing Initialize Vulkan");
  if (this->validation.enable_validation_layers) {
    this->validation.destroy_debug_utils_messenger_ext(
        this->instance, this->validation.debug_messenger, nullptr);
  }
  vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
  vkDestroyInstance(this->instance, nullptr);
}

void CE::BaseInitializeVulkan::create_instance() {
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
  Log::text(Log::Style::char_leader,
            app_info.pApplicationName,
            app_info.applicationVersion,
            "-",
            app_info.pEngineName,
            app_info.engineVersion,
            "-",
            "Vulkan",
            1.3);

  const std::vector<const char *> extensions = get_required_extensions();

    VkInstanceCreateInfo create_info{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                     .pNext = nullptr,
                     .pApplicationInfo = &app_info,
                     .enabledLayerCount = 0,
                     .enabledExtensionCount =
                       static_cast<uint32_t>(extensions.size()),
                     .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
  if (this->validation.enable_validation_layers) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(this->validation.validation.size());
    create_info.ppEnabledLayerNames = this->validation.validation.data();

    this->validation.populate_debug_messenger_create_info(debug_create_info);
    create_info.pNext = &debug_create_info;
  }
    CE::vulkan_result(vkCreateInstance, &create_info, nullptr, &this->instance);
}

void CE::BaseInitializeVulkan::create_surface(GLFWwindow *window) {
  Log::text("{ [ ] }", "Surface");
    CE::vulkan_result(
      glfwCreateWindowSurface, this->instance, window, nullptr, &this->surface);
}

std::vector<const char *> CE::BaseInitializeVulkan::get_required_extensions() const {
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
