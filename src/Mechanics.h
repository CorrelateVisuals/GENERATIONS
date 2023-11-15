#pragma once
#include "CapitalEngine.h"
#include "Pipelines.h"
#include "Resources.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanMechanics {
 public:
  VulkanMechanics();
  ~VulkanMechanics();

  CE::InitializeVulkan initVulkan{};
  CE::Queues queues{};

  struct Device : public CE::Device {
    Device(const CE::InitializeVulkan& initVulkan,
           CE::Queues& queues,
           CE::Swapchain& swapchain) {
      CE::Device::baseDevice = this;

      features.tessellationShader = VK_TRUE;
      features.sampleRateShading = VK_TRUE;
      features.depthClamp = VK_TRUE;
      features.depthBiasClamp = VK_TRUE;
      features.fillModeNonSolid = VK_TRUE;
      features.wideLines = VK_TRUE;
      features.samplerAnisotropy = VK_TRUE;
      features.shaderInt64 = VK_TRUE;

      pickPhysicalDevice(initVulkan, queues, swapchain);
      createLogicalDevice(initVulkan, queues);
    };
    ~Device() { destroyDevice(); }
  } mainDevice;

  struct SynchronizationObjects : public CE::SynchronizationObjects {
    SynchronizationObjects() { create(); }
  } syncObjects;

  struct Swapchain : public CE::Swapchain {
    Swapchain(const VkSurfaceKHR& surface, const CE::Queues& queues) {
      create(surface, queues);
    }
    void recreate(const VkSurfaceKHR& surface,
                  const CE::Queues& queues,
                  SynchronizationObjects& syncObjects,
                  Pipelines& _pipelines,
                  Resources& _resources);
  } swapchain;
};
