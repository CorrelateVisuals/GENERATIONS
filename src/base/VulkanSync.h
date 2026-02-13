#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanCore.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"

class Resources;
class Pipelines;

namespace CE {

class Swapchain;

class CommandBuffers {
public:
  VkCommandPool pool{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> graphics{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> compute{};
  static VkCommandBuffer singularCommandBuffer;

  CommandBuffers() = default;
  virtual ~CommandBuffers();
  static void beginSingularCommands(const VkCommandPool &commandPool,
                                    const VkQueue &queue);
  static void endSingularCommands(const VkCommandPool &commandPool, const VkQueue &queue);
  virtual void recordComputeCommandBuffer(Resources &resources,
                                          Pipelines &pipelines,
                                          const uint32_t imageIndex) = 0;
  virtual void recordGraphicsCommandBuffer(Swapchain &swapchain,
                                           Resources &resources,
                                           Pipelines &pipelines,
                                           const uint32_t imageIndex) = 0;

protected:
  void createPool(const Queues::FamilyIndices &familyIndices);
  void
  createBuffers(std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> &commandBuffers) const;
};

struct CommandInterface {
  VkCommandBuffer &commandBuffer;
  const VkCommandPool &commandPool;
  const VkQueue &queue;

  CommandInterface(VkCommandBuffer &commandBufferRef,
                   const VkCommandPool &commandPoolRef,
                   const VkQueue &queueRef)
      : commandBuffer(commandBufferRef), commandPool(commandPoolRef), queue(queueRef) {}
};

class SynchronizationObjects {
public:
  SynchronizationObjects() = default;
  ~SynchronizationObjects() {
    destroy();
  };

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
  virtual ~Swapchain() {
    destroy();
  };
  SupportDetails checkSupport(const VkPhysicalDevice &physicalDevice,
                              const VkSurfaceKHR &surface);

protected:
  void create(const VkSurfaceKHR &surface, const Queues &queues);
  void recreate(const VkSurfaceKHR &surface,
                const Queues &queues,
                SynchronizationObjects &syncObjects);

private:
  void destroy();
  VkSurfaceFormatKHR
  pickSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) const;
  VkPresentModeKHR
  pickPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) const;
  VkExtent2D pickExtent(GLFWwindow *window,
                        const VkSurfaceCapabilitiesKHR &capabilities) const;
  uint32_t getImageCount(const Swapchain::SupportDetails &swapchainSupport) const;
};

} // namespace CE
