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

  struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;
    const std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const VkPhysicalDeviceFeatures features{.tessellationShader = VK_TRUE,
                                            .sampleRateShading = VK_TRUE,
                                            .depthClamp = VK_TRUE,
                                            .depthBiasClamp = VK_TRUE,
                                            .fillModeNonSolid = VK_TRUE,
                                            .wideLines = VK_TRUE,
                                            .samplerAnisotropy = VK_TRUE,
                                            .shaderInt64 = VK_TRUE};
  } mainDevice;

  struct Queues {
    VkQueue graphics;
    VkQueue compute;
    VkQueue present;

    struct FamilyIndices {
      std::optional<uint32_t> graphicsAndComputeFamily;
      std::optional<uint32_t> presentFamily;
      bool isComplete() const {
        return graphicsAndComputeFamily.has_value() &&
               presentFamily.has_value();
      }
    } familyIndices;
  } queues;

  struct SwapChain {
    VkSwapchainKHR swapChain;
    VkFormat imageFormat;
    VkExtent2D extent;
    std::vector<CE::Image> images;
    std::vector<VkFramebuffer> framebuffers;

    struct SupportDetails {
      VkSurfaceCapabilitiesKHR capabilities{};
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
    } supportDetails;
  } swapChain;

  struct SynchronizationObjects {
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> computeInFlightFences;
    uint32_t currentFrame = 0;
  } syncObjects;

 public:
  void setupVulkan(Pipelines& _pipelines, Resources& _resources);
  void recreateSwapChain(Pipelines& _pipelines, Resources& _resources);
  void cleanupSwapChain(Resources& _resources);
  void createSyncObjects();

 private:
  void createInstance();
  void createSurface(GLFWwindow* window);
  void pickPhysicalDevice(Resources::MultiSamplingImage& msaaImage);
  void createLogicalDevice();
  void createSwapChain();
  void createCommandPool(VkCommandPool* commandPool);

  std::vector<const char*> getRequiredExtensions();
  bool isDeviceSuitable(VkPhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
  VkSampleCountFlagBits getMaxUsableSampleCount();
  SwapChain::SupportDetails querySwapChainSupport(
      VkPhysicalDevice physicalDevice);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
  Queues::FamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
};
