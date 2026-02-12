#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace CE {

// Constants
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// Forward declarations
class Queues;
class Image;

/**
 * @brief Command buffer interface for Vulkan command recording
 */
struct CommandInterface {
  VkCommandBuffer& commandBuffer;
  const VkCommandPool& commandPool;
  const VkQueue& queue;

  CommandInterface(VkCommandBuffer& commandBuffer,
                   const VkCommandPool& commandPool,
                   const VkQueue& queue)
      : commandBuffer(commandBuffer), commandPool(commandPool), queue(queue) {}
};

/**
 * @brief Command buffer management for graphics and compute
 */
class CommandBuffers {
 public:
  VkCommandPool pool{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> graphics{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> compute{};
  static VkCommandBuffer singularCommandBuffer;

  CommandBuffers() = default;
  virtual ~CommandBuffers();
  
  static void beginSingularCommands(const VkCommandPool& commandPool,
                                    const VkQueue& queue);
  static void endSingularCommands(const VkCommandPool& commandPool,
                                  const VkQueue& queue);
  
  // Pure virtual methods for derived classes to implement
  virtual void recordComputeCommandBuffer(class Resources& resources,
                                          class Pipelines& pipelines,
                                          const uint32_t imageIndex) = 0;
  virtual void recordGraphicsCommandBuffer(Swapchain& swapchain,
                                           class Resources& resources,
                                           class Pipelines& pipelines,
                                           const uint32_t imageIndex) = 0;

 protected:
  void createPool(const Queues::FamilyIndices& familyIndices);
  void createBuffers(
      std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>& commandBuffers) const;
};

/**
 * @brief Synchronization objects for frame coordination
 * 
 * Manages semaphores and fences for graphics/compute synchronization.
 */
class SynchronizationObjects {
 public:
  SynchronizationObjects() = default;
  ~SynchronizationObjects() { destroy(); }

  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{};
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{};
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> computeFinishedSemaphores{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> graphicsInFlightFences{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> computeInFlightFences{};
  uint32_t currentFrame = 0;

 protected:
  void create();

 private:
  void destroy() const;
};

/**
 * @brief Swapchain management for presentation
 * 
 * Handles swapchain creation, recreation, and image management.
 */
class Swapchain {
 public:
  VkSwapchainKHR swapchain{};
  VkExtent2D extent{};
  VkFormat imageFormat{};
  std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> images{};
  std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> framebuffers{};

  struct SupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
  } supportDetails{};

  Swapchain() = default;
  virtual ~Swapchain() { destroy(); }
  
  const SupportDetails checkSupport(const VkPhysicalDevice& physicalDevice,
                                    const VkSurfaceKHR& surface);

 protected:
  void create(const VkSurfaceKHR& surface, const Queues& queues);
  void recreate(const VkSurfaceKHR& surface,
                const Queues& queues,
                SynchronizationObjects& syncObjects);

 private:
  void destroy();
  const VkSurfaceFormatKHR pickSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  const VkPresentModeKHR pickPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  const VkExtent2D pickExtent(
      struct GLFWwindow* window,
      const VkSurfaceCapabilitiesKHR& capabilities) const;
  const uint32_t getImageCount(
      const Swapchain::SupportDetails& swapchainSupport) const;
};

}  // namespace CE
