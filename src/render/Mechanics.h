#pragma once
#include "base/VulkanDevice.h"
#include "base/VulkanSync.h"

class Pipelines;
class Resources;

class VulkanMechanics {
public:
  VulkanMechanics();
  ~VulkanMechanics();

  CE::initialize_vulkan init_vulkan{};
  CE::queues queues{};

  struct device : public CE::device {
    device(const CE::initialize_vulkan &init_vulkan,
           CE::queues &queues,
           CE::Swapchain &swapchain) {
      CE::device::base_device = this;

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
    ~device() {
      destroy_device();
    }
  };

  struct SynchronizationObjects : public CE::SynchronizationObjects {
    SynchronizationObjects() {
      create();
    }
  };

  struct Swapchain : public CE::Swapchain {
    Swapchain() = default;
    void initialize(const VkSurfaceKHR &surface, const CE::queues &queues) {
      create(surface, queues);
    }
    void recreate(const VkSurfaceKHR &surface,
                  const CE::queues &queues,
                  SynchronizationObjects &sync_objects,
                  Pipelines &pipelines,
                  Resources &resources);
  };

private:
  Swapchain swapchain_support;

public:
  device main_device;
  Swapchain swapchain;
  SynchronizationObjects sync_objects;
};
