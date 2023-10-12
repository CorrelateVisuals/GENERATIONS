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

  struct Swapchain : public CE::Swapchain {
    std::vector<CE::Image> images;
    std::vector<VkFramebuffer> framebuffers;
    VkFormat imageFormat;
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapchain;

    void create(Device& device, VkSurfaceKHR& surface);
    void destroy(VkDevice& logicalDevice);
    SupportDetails checkSupport(VkPhysicalDevice physicalDevice,
                                VkSurfaceKHR& surface);
    VkSurfaceFormatKHR pickSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR pickPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D pickExtent(VkSurfaceCapabilitiesKHR& capabilities);
  } swapchain;

  struct Queues : public CE::Queues {
    VkQueue graphics;
    VkQueue compute;
    VkQueue present;
  } queues;

  struct SynchronizationObjects {
    void create(VkDevice& logicalDevice);
    void destroy(VkDevice& logicalDevice);
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> graphicsInFlightFences;
    std::vector<VkFence> computeInFlightFences;
    uint32_t currentFrame = 0;
  } syncObjects;

 public:
  void setupVulkan(Pipelines& _pipelines, Resources& _resources);

  void recreateswapchain(Pipelines& _pipelines, Resources& _resources);

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

  static CE::Queues::FamilyIndices findQueueFamilies(
      const VkPhysicalDevice& physicalDevice,
      const VkSurfaceKHR& surface);
};
