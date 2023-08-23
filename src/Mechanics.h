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

  struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;
    const std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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
    std::vector<VkImage> images;
    VkFormat imageFormat;
    std::vector<VkImageView> imageViews;
    VkExtent2D extent;
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
  void setupVulkan();

  void recreateSwapChain();
  void cleanupSwapChain();

  void createSyncObjects();

  template <typename Checkresult, typename... Args>
  void result(Checkresult vkResult, Args&&... args) {
    using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
    std::string objectName = typeid(ObjectType).name();

    VkResult result = vkResult(std::forward<Args>(args)...);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName +
                               "!");
    }
  }

 private:
  void compileShaders();
  void createInstance(ValidationLayers& validation);
  void createSurface(GLFWwindow* window);
  void pickPhysicalDevice(Pipelines::Graphics::MultiSampling& msaa);
  void createLogicalDevice(ValidationLayers& validation);
  void createSwapChain();
  void createSwapChainImageViews(Resources& resources);
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
