#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "base/ValidationLayers.h"
#include "control/Window.h"

namespace CE {

class Swapchain;

class Queues {
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

  Queues() = default;
  virtual ~Queues() = default;
  FamilyIndices find_queue_families(const VkPhysicalDevice &physical_device,
                                    const VkSurfaceKHR &surface) const;
};

class InitializeVulkan {
public:
  VkSurfaceKHR surface{};
  VkInstance instance{};
  ValidationLayers validation{};

  InitializeVulkan();
  InitializeVulkan(const InitializeVulkan &) = delete;
  InitializeVulkan &operator=(const InitializeVulkan &) = delete;
  InitializeVulkan(InitializeVulkan &&) = delete;
  InitializeVulkan &operator=(InitializeVulkan &&) = delete;
  virtual ~InitializeVulkan();

private:
  void create_instance();
  void create_surface(GLFWwindow *window);
  std::vector<const char *> get_required_extensions() const;
};

class Device {
public:
  VkPhysicalDevice physical_device{VK_NULL_HANDLE};
  VkSampleCountFlagBits max_usable_sample_count{VK_SAMPLE_COUNT_1_BIT};
  VkDevice logical_device{VK_NULL_HANDLE};

  static Device *base_device;

  Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  Device(Device &&) = delete;
  Device &operator=(Device &&) = delete;
  virtual ~Device() {
    destroy_device();
  }

  void maybe_log_gpu_runtime_sample();

protected:
  VkPhysicalDeviceFeatures features{};
  void pick_physical_device(const InitializeVulkan &init_vulkan,
                            Queues &queues,
                            Swapchain &swapchain);
  void create_logical_device(const InitializeVulkan &init_vulkan, Queues &queues);
  void destroy_device();

private:
  VkPhysicalDeviceProperties properties_{};
  bool memory_budget_supported_{false};
  VkDeviceSize device_local_heap_total_bytes_{0};
  std::chrono::steady_clock::time_point last_gpu_runtime_log_{};
  bool gpu_runtime_log_initialized_{false};
  std::vector<const char *> extensions_{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  static std::vector<VkDevice> destroyed_devices;

  std::vector<VkDeviceQueueCreateInfo> fill_queue_create_infos(const Queues &queues) const;
    VkDeviceCreateInfo
    get_device_create_info(const std::vector<VkDeviceQueueCreateInfo> &queue_create_infos)
      const;
  void set_validation_layers(const InitializeVulkan &init_vulkan,
                             VkDeviceCreateInfo &create_info);
  std::vector<VkPhysicalDevice> fill_devices(const InitializeVulkan &init_vulkan) const;
  bool is_device_suitable(const VkPhysicalDevice &physical_device,
                          Queues &queues,
                          const InitializeVulkan &init_vulkan,
                          Swapchain &swapchain);
  void get_max_usable_sample_count();
  bool check_device_extension_support(const VkPhysicalDevice &physical_device) const;
};

} // namespace CE
