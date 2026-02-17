#pragma once

// Vulkan instance/device/queue bootstrap layer.
// Exists to own GPU selection, logical device creation, and queue discovery.

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vulkan_base/VulkanBaseValidationLayers.h"
#include "control/Window.h"

namespace CE {

class BaseSwapchain;

class BaseQueues {
public:
  VkQueue graphics_queue{};
  VkQueue compute_queue{};
  VkQueue present_queue{};

  struct FamilyIndices {
    std::optional<uint32_t> graphics_and_compute_family{};
    std::optional<uint32_t> present_family{};
    bool is_complete() const {
      return graphics_and_compute_family.has_value() && present_family.has_value();
    }
  };

  FamilyIndices indices{};

  BaseQueues() = default;
  virtual ~BaseQueues() = default;
  FamilyIndices find_queue_families(const VkPhysicalDevice &physical_device,
                                    const VkSurfaceKHR &surface) const;
};

class BaseInitializeVulkan {
public:
  VkSurfaceKHR surface{};
  VkInstance instance{};
  BaseValidationLayers validation{};

  BaseInitializeVulkan();
  BaseInitializeVulkan(const BaseInitializeVulkan &) = delete;
  BaseInitializeVulkan &operator=(const BaseInitializeVulkan &) = delete;
  BaseInitializeVulkan(BaseInitializeVulkan &&) = delete;
  BaseInitializeVulkan &operator=(BaseInitializeVulkan &&) = delete;
  virtual ~BaseInitializeVulkan();

private:
  void create_instance();
  void create_surface(GLFWwindow *window);
  std::vector<const char *> get_required_extensions() const;
};

class BaseDevice {
public:
  VkPhysicalDevice physical_device{VK_NULL_HANDLE};
  VkSampleCountFlagBits max_usable_sample_count{VK_SAMPLE_COUNT_1_BIT};
  VkDevice logical_device{VK_NULL_HANDLE};

  static BaseDevice *base_device;

  BaseDevice() = default;
  BaseDevice(const BaseDevice &) = delete;
  BaseDevice &operator=(const BaseDevice &) = delete;
  BaseDevice(BaseDevice &&) = delete;
  BaseDevice &operator=(BaseDevice &&) = delete;
  virtual ~BaseDevice() {
    destroy_device();
  }

  void maybe_log_gpu_runtime_sample();

protected:
  VkPhysicalDeviceFeatures features{};
  void pick_physical_device(const BaseInitializeVulkan &init_vulkan,
                            BaseQueues &queues,
                            BaseSwapchain &swapchain);
  void create_logical_device(const BaseInitializeVulkan &init_vulkan, BaseQueues &queues);
  void destroy_device();

private:
  VkPhysicalDeviceProperties properties_{};
  bool memory_budget_supported_{false};
  VkDeviceSize device_local_heap_total_bytes_{0};
  std::chrono::steady_clock::time_point last_gpu_runtime_log_{};
  bool gpu_runtime_log_initialized_{false};
  std::vector<const char *> extensions_{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  static std::vector<VkDevice> destroyed_devices;

  std::vector<VkDeviceQueueCreateInfo> fill_queue_create_infos(const BaseQueues &queues) const;
    VkDeviceCreateInfo
    get_device_create_info(const std::vector<VkDeviceQueueCreateInfo> &queue_create_infos)
      const;
  void set_validation_layers(const BaseInitializeVulkan &init_vulkan,
                             VkDeviceCreateInfo &create_info);
  std::vector<VkPhysicalDevice> fill_devices(const BaseInitializeVulkan &init_vulkan) const;
  bool is_device_suitable(const VkPhysicalDevice &physical_device,
                          BaseQueues &queues,
                          const BaseInitializeVulkan &init_vulkan,
                          BaseSwapchain &swapchain);
  void get_max_usable_sample_count();
  bool check_device_extension_support(const VkPhysicalDevice &physical_device) const;
};

} // namespace CE
