#pragma once

// Runtime Vulkan mechanics bundle.
// Exists to aggregate initialization, queue families, swapchain, and sync objects.
#include "vulkan_base/VulkanBaseDevice.h"
#include "vulkan_base/VulkanBaseSync.h"

class Pipelines;
class VulkanResources;

class VulkanMechanics {
public:
  VulkanMechanics();
  VulkanMechanics(const VulkanMechanics &) = delete;
  VulkanMechanics &operator=(const VulkanMechanics &) = delete;
  VulkanMechanics(VulkanMechanics &&) = delete;
  VulkanMechanics &operator=(VulkanMechanics &&) = delete;
  ~VulkanMechanics();

  CE::BaseInitializeVulkan init_vulkan{};
  CE::BaseQueues queues{};

  struct BaseDevice : public CE::BaseDevice {
    BaseDevice(const CE::BaseInitializeVulkan &init_vulkan,
           CE::BaseQueues &queues,
           CE::BaseSwapchain &swapchain) {
      CE::BaseDevice::base_device = this;

      features.tessellationShader = VK_TRUE;
      features.sampleRateShading = VK_TRUE;
      features.depthClamp = VK_TRUE;
      features.depthBiasClamp = VK_TRUE;
      features.fillModeNonSolid = VK_TRUE;
      features.wideLines = VK_TRUE;
      features.samplerAnisotropy = VK_TRUE;
      features.shaderInt64 = VK_TRUE;

      pick_physical_device(init_vulkan, queues, swapchain);
      create_logical_device(init_vulkan, queues);
    };
    ~BaseDevice() = default;
  };

  struct BaseSynchronizationObjects : public CE::BaseSynchronizationObjects {
    BaseSynchronizationObjects() {
      create();
    }
  };

  struct BaseSwapchain : public CE::BaseSwapchain {
    BaseSwapchain() = default;
    void initialize(const VkSurfaceKHR &surface, const CE::BaseQueues &queues) {
      create(surface, queues);
    }
    void recreate(const VkSurfaceKHR &surface,
                  const CE::BaseQueues &queues,
                  BaseSynchronizationObjects &sync_objects,
                  Pipelines &pipelines,
                  VulkanResources &resources);
  };

private:
  BaseSwapchain swapchain_support;

public:
  BaseDevice main_device;
  BaseSwapchain swapchain;
  BaseSynchronizationObjects sync_objects;
};
