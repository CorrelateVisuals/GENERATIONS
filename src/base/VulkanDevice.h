#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "platform/ValidationLayers.h"
#include "platform/Window.h"

namespace CE {

class Swapchain;

class Queues {
public:
  VkQueue graphics{};
  VkQueue compute{};
  VkQueue present{};

  struct FamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily{};
    std::optional<uint32_t> presentFamily{};
    bool isComplete() const {
      return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
  };

  FamilyIndices familyIndices{};

  Queues() = default;
  virtual ~Queues() = default;
  FamilyIndices findQueueFamilies(const VkPhysicalDevice &physicalDevice,
                                  const VkSurfaceKHR &surface) const;
};

class InitializeVulkan {
public:
  VkSurfaceKHR surface{};
  VkInstance instance{};
  ValidationLayers validation{};

  InitializeVulkan();
  virtual ~InitializeVulkan();

private:
  void createInstance();
  void createSurface(GLFWwindow *window);
  std::vector<const char *> getRequiredExtensions() const;
};

class Device {
public:
  VkPhysicalDevice physical{VK_NULL_HANDLE};
  VkPhysicalDeviceFeatures features{};
  VkSampleCountFlagBits maxUsableSampleCount{VK_SAMPLE_COUNT_1_BIT};
  VkDevice logical{VK_NULL_HANDLE};

  static Device *baseDevice;

  Device() = default;
  virtual ~Device() {
    destroyDevice();
  }

protected:
  void pickPhysicalDevice(const InitializeVulkan &initVulkan,
                          Queues &queues,
                          Swapchain &swapchain);
  void createLogicalDevice(const InitializeVulkan &initVulkan, Queues &queues);
  void destroyDevice();

private:
  VkPhysicalDeviceProperties properties{};
  std::vector<const char *> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  static std::vector<VkDevice> destroyedDevices;

  std::vector<VkDeviceQueueCreateInfo> fillQueueCreateInfos(const Queues &queues) const;
  VkDeviceCreateInfo
  getDeviceCreateInfo(const std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos) const;
  void setValidationLayers(const InitializeVulkan &initVulkan,
                           VkDeviceCreateInfo &createInfo);
  std::vector<VkPhysicalDevice> fillDevices(const InitializeVulkan &initVulkan) const;
  bool isDeviceSuitable(const VkPhysicalDevice &physical,
                        Queues &queues,
                        const InitializeVulkan &initVulkan,
                        Swapchain &swapchain);
  void getMaxUsableSampleCount();
  bool checkDeviceExtensionSupport(const VkPhysicalDevice &physical) const;
};

} // namespace CE
