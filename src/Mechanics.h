#pragma once
#include <GLFW/glfw3.h>

#include "CapitalEngine.h"
#include "Pipelines.h"
#include "Resources.h"
// #include "ValidationLayers.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanMechanics {
 public:
  VulkanMechanics();
  ~VulkanMechanics();

  struct Instance : public CE::Instance {
  } instance;

  // VkSurfaceKHR surface;
  // VkInstance instance;
  // ValidationLayers validation;

  struct Device : public CE::Device {
    Device() {
      features.tessellationShader = VK_TRUE;
      features.sampleRateShading = VK_TRUE;
      features.depthClamp = VK_TRUE;
      features.depthBiasClamp = VK_TRUE;
      features.fillModeNonSolid = VK_TRUE;
      features.wideLines = VK_TRUE;
      features.samplerAnisotropy = VK_TRUE;
      features.shaderInt64 = VK_TRUE;
    };
  } mainDevice;

  struct Queues : public CE::Queues {
  } queues;

  struct SynchronizationObjects : public CE::SynchronizationObjects {
  } syncObjects;

  struct Swapchain : public CE::Swapchain {
    void recreate(const VkSurfaceKHR& surface,
                  const Queues& queues,
                  SynchronizationObjects& syncObjects,
                  Pipelines& _pipelines,
                  Resources& _resources);
  } swapchain;

 public:
  void setupVulkan(Pipelines& _pipelines, Resources& _resources);

 private:
  // void createInstance();
  // void createSurface(GLFWwindow* window);
  // std::vector<const char*> getRequiredExtensions();

  void pickPhysicalDevice(VkSampleCountFlagBits& msaaImageSamples);
  void createLogicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
  VkSampleCountFlagBits getMaxUsableSampleCount();
};
