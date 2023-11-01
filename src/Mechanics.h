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
      CE::baseDevice->setBaseDevice(*this);
    };
    ~Device() { destroyDevice(); }
  } mainDevice;

  struct SynchronizationObjects : public CE::SynchronizationObjects {
    ~SynchronizationObjects() { destroy(MAX_FRAMES_IN_FLIGHT); }
  } syncObjects;

  struct Swapchain : public CE::Swapchain {
    Swapchain(const VkSurfaceKHR& surface, const CE::Queues& queues) {
      create(surface, queues, MAX_FRAMES_IN_FLIGHT);
    }
    void recreate(const VkSurfaceKHR& surface,
                  const CE::Queues& queues,
                  SynchronizationObjects& syncObjects,
                  Pipelines& _pipelines,
                  Resources& _resources);
  } swapchain;
};
