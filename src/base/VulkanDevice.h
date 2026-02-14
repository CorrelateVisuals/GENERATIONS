#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "platform/ValidationLayers.h"
#include "platform/Window.h"

namespace CE {

class Swapchain;

class queues {
public:
  VkQueue graphics_queue{};
  VkQueue compute_queue{};
  VkQueue present_queue{};

  struct family_indices {
    std::optional<uint32_t> graphics_and_compute_family{};
    std::optional<uint32_t> present_family{};
    bool is_complete() const {
      return graphics_and_compute_family.has_value() && present_family.has_value();
    }
  };
  using FamilyIndices = family_indices;

  family_indices indices{};

  queues() = default;
  virtual ~queues() = default;
  family_indices find_queue_families(const VkPhysicalDevice &physical_device,
                                    const VkSurfaceKHR &surface) const;
};

class initialize_vulkan {
public:
  VkSurfaceKHR surface{};
  VkInstance instance{};
  ValidationLayers validation{};

  initialize_vulkan();
  virtual ~initialize_vulkan();

private:
  void create_instance();
  void create_surface(GLFWwindow *window);
  std::vector<const char *> get_required_extensions() const;
};

class device {
public:
  VkPhysicalDevice physical_device{VK_NULL_HANDLE};
  VkSampleCountFlagBits max_usable_sample_count{VK_SAMPLE_COUNT_1_BIT};
  VkDevice logical_device{VK_NULL_HANDLE};

  static device *base_device;

  device() = default;
  virtual ~device() {
    destroy_device();
  }

protected:
  VkPhysicalDeviceFeatures features{};
  void pick_physical_device(const initialize_vulkan &init_vulkan,
                            queues &queues,
                            Swapchain &swapchain);
  void create_logical_device(const initialize_vulkan &init_vulkan, queues &queues);
  void destroy_device();

private:
  VkPhysicalDeviceProperties properties_{};
  std::vector<const char *> extensions_{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  static std::vector<VkDevice> destroyed_devices;

  std::vector<VkDeviceQueueCreateInfo> fill_queue_create_infos(const queues &queues) const;
    VkDeviceCreateInfo
    get_device_create_info(const std::vector<VkDeviceQueueCreateInfo> &queue_create_infos)
      const;
  void set_validation_layers(const initialize_vulkan &init_vulkan,
                             VkDeviceCreateInfo &create_info);
  std::vector<VkPhysicalDevice> fill_devices(const initialize_vulkan &init_vulkan) const;
  bool is_device_suitable(const VkPhysicalDevice &physical_device,
                          queues &queues,
                          const initialize_vulkan &init_vulkan,
                          Swapchain &swapchain);
  void get_max_usable_sample_count();
  bool check_device_extension_support(const VkPhysicalDevice &physical_device) const;
};

// Type aliases for compatibility
using InitializeVulkan = initialize_vulkan;
using Queues = queues;
using Device = device;

} // namespace CE
