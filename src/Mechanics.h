#pragma once
#include <GLFW/glfw3.h>

#include "CapitalEngine.h"
#include "Pipelines.h"
#include "Resources.h"
#include "ValidationLayers.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanMechanics {
 public:
  VulkanMechanics();
  ~VulkanMechanics();

  VkSurfaceKHR surface;
  VkInstance instance;
  ValidationLayers validation;

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

  struct SwapChain : public CE::Swapchain {
  } swapChain;

  struct Queues : public CE::Queues {
    VkQueue graphics;
    VkQueue compute;
    VkQueue present;
  } queues;

  struct SynchronizationObjects : public CE::SynchronizationObjects {
    uint32_t currentFrame = 0;
  } syncObjects;

 public:
  void setupVulkan(Pipelines& _pipelines, Resources& _resources);

  void recreateSwapChain(Pipelines& _pipelines, Resources& _resources);

  void createSyncObjects();

 private:
  void createInstance();
  void createSurface(GLFWwindow* window);

  void pickPhysicalDevice(VkSampleCountFlagBits& msaaImageSamples);
  void createLogicalDevice();
  std::vector<const char*> getRequiredExtensions();
  bool isDeviceSuitable(VkPhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
  VkSampleCountFlagBits getMaxUsableSampleCount();

  void createCommandPool(VkCommandPool* commandPool);

  void createSwapChain();
  SwapChain::SupportDetails querySwapChainSupport(
      VkPhysicalDevice physicalDevice);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  CE::Queues::FamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
};
