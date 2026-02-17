#pragma once

// Command buffer, synchronization, and swapchain synchronization primitives.
// Exists to define frame-in-flight execution contract and submission scaffolding.

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanBaseDescriptor.h"
#include "VulkanBaseDevice.h"
#include "VulkanBaseResources.h"

class VulkanResources;
class Pipelines;

namespace CE {

class BaseSwapchain;

class BaseCommandBuffers {
public:
  VkCommandPool pool{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> graphics{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> compute{};
  static VkCommandBuffer singular_command_buffer;

  BaseCommandBuffers() = default;
  BaseCommandBuffers(const BaseCommandBuffers &) = delete;
  BaseCommandBuffers &operator=(const BaseCommandBuffers &) = delete;
  BaseCommandBuffers(BaseCommandBuffers &&) = delete;
  BaseCommandBuffers &operator=(BaseCommandBuffers &&) = delete;
  virtual ~BaseCommandBuffers();
  static void begin_singular_commands(const VkCommandPool &command_pool,
                                    const VkQueue &queue);
  static void end_singular_commands(const VkCommandPool &command_pool,
                                    const VkQueue &queue);
  virtual void record_compute_command_buffer(VulkanResources &resources,
                                             Pipelines &pipelines,
                                             const uint32_t frame_index) = 0;
  virtual void record_graphics_command_buffer(BaseSwapchain &swapchain,
                                              VulkanResources &resources,
                                              Pipelines &pipelines,
                                              const uint32_t frame_index,
                                              const uint32_t image_index) = 0;

protected:
  void create_pool(const BaseQueues::FamilyIndices &family_indices);
  void create_buffers(
      std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> &command_buffers) const;
};

class BaseSingleUseCommands {
public:
  BaseSingleUseCommands(const VkCommandPool &command_pool, const VkQueue &queue);
  BaseSingleUseCommands(const BaseSingleUseCommands &) = delete;
  BaseSingleUseCommands &operator=(const BaseSingleUseCommands &) = delete;
  BaseSingleUseCommands(BaseSingleUseCommands &&) = delete;
  BaseSingleUseCommands &operator=(BaseSingleUseCommands &&) = delete;
  ~BaseSingleUseCommands();

  VkCommandBuffer &command_buffer();
  void submit_and_wait();

private:
  const VkCommandPool &command_pool_;
  const VkQueue &queue_;
  bool submitted_{false};
};

struct BaseCommandInterface {
  VkCommandBuffer &command_buffer;
  const VkCommandPool &command_pool;
  const VkQueue &queue;

  BaseCommandInterface(VkCommandBuffer &command_buffer_ref,
                   const VkCommandPool &command_pool_ref,
                   const VkQueue &queue_ref)
      : command_buffer(command_buffer_ref),
        command_pool(command_pool_ref),
        queue(queue_ref) {}
};

class BaseSynchronizationObjects {
public:
  BaseSynchronizationObjects() = default;
  BaseSynchronizationObjects(const BaseSynchronizationObjects &) = delete;
  BaseSynchronizationObjects &operator=(const BaseSynchronizationObjects &) = delete;
  BaseSynchronizationObjects(BaseSynchronizationObjects &&) = delete;
  BaseSynchronizationObjects &operator=(BaseSynchronizationObjects &&) = delete;
  ~BaseSynchronizationObjects() {
    destroy();
  };

  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> image_available_semaphores{};
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> render_finished_semaphores{};
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> compute_finished_semaphores{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> graphics_in_flight_fences{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> compute_in_flight_fences{};
  uint32_t current_frame = 0;

protected:
  void create();

private:
  void destroy();
};

class BaseSwapchain {
public:
  VkSwapchainKHR swapchain{};
  VkExtent2D extent{};
  VkFormat image_format{};
  std::array<CE::BaseImage, MAX_FRAMES_IN_FLIGHT> images{};
  std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> framebuffers{};

  struct SupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> present_modes{};
  };

  BaseSwapchain() = default;
  BaseSwapchain(const BaseSwapchain &) = delete;
  BaseSwapchain &operator=(const BaseSwapchain &) = delete;
  BaseSwapchain(BaseSwapchain &&) = delete;
  BaseSwapchain &operator=(BaseSwapchain &&) = delete;
  virtual ~BaseSwapchain() {
    destroy();
  };
  SupportDetails check_support(const VkPhysicalDevice &physical_device,
                               const VkSurfaceKHR &surface);

protected:
  void create(const VkSurfaceKHR &surface, const BaseQueues &queues);
  void recreate(const VkSurfaceKHR &surface,
                const BaseQueues &queues,
                BaseSynchronizationObjects &sync_objects);

private:
  SupportDetails support_details{};
  void destroy();
  VkSurfaceFormatKHR
  pick_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats) const;
  VkPresentModeKHR pick_present_mode(
      const std::vector<VkPresentModeKHR> &available_present_modes) const;
  VkExtent2D pick_extent(GLFWwindow *window,
                        const VkSurfaceCapabilitiesKHR &capabilities) const;
  uint32_t get_image_count(const BaseSwapchain::SupportDetails &swapchain_support) const;
};

} // namespace CE
