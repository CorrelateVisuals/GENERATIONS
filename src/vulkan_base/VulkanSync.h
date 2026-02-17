#pragma once

// Command buffer, synchronization, and swapchain synchronization primitives.
// Exists to define frame-in-flight execution contract and submission scaffolding.

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanDescriptor.h"
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
  static VkCommandBuffer singular_command_buffer;

  CommandBuffers() = default;
  CommandBuffers(const CommandBuffers &) = delete;
  CommandBuffers &operator=(const CommandBuffers &) = delete;
  CommandBuffers(CommandBuffers &&) = delete;
  CommandBuffers &operator=(CommandBuffers &&) = delete;
  virtual ~CommandBuffers();
  static void begin_singular_commands(const VkCommandPool &command_pool,
                                    const VkQueue &queue);
  static void end_singular_commands(const VkCommandPool &command_pool,
                                    const VkQueue &queue);
  virtual void record_compute_command_buffer(Resources &resources,
                                             Pipelines &pipelines,
                                             const uint32_t frame_index) = 0;
  virtual void record_graphics_command_buffer(Swapchain &swapchain,
                                              Resources &resources,
                                              Pipelines &pipelines,
                                              const uint32_t frame_index,
                                              const uint32_t image_index) = 0;

protected:
  void create_pool(const Queues::FamilyIndices &family_indices);
  void create_buffers(
      std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> &command_buffers) const;
};

class SingleUseCommands {
public:
  SingleUseCommands(const VkCommandPool &command_pool, const VkQueue &queue);
  SingleUseCommands(const SingleUseCommands &) = delete;
  SingleUseCommands &operator=(const SingleUseCommands &) = delete;
  SingleUseCommands(SingleUseCommands &&) = delete;
  SingleUseCommands &operator=(SingleUseCommands &&) = delete;
  ~SingleUseCommands();

  VkCommandBuffer &command_buffer();
  void submit_and_wait();

private:
  const VkCommandPool &command_pool_;
  const VkQueue &queue_;
  bool submitted_{false};
};

struct CommandInterface {
  VkCommandBuffer &command_buffer;
  const VkCommandPool &command_pool;
  const VkQueue &queue;

  CommandInterface(VkCommandBuffer &command_buffer_ref,
                   const VkCommandPool &command_pool_ref,
                   const VkQueue &queue_ref)
      : command_buffer(command_buffer_ref),
        command_pool(command_pool_ref),
        queue(queue_ref) {}
};

class SynchronizationObjects {
public:
  SynchronizationObjects() = default;
  SynchronizationObjects(const SynchronizationObjects &) = delete;
  SynchronizationObjects &operator=(const SynchronizationObjects &) = delete;
  SynchronizationObjects(SynchronizationObjects &&) = delete;
  SynchronizationObjects &operator=(SynchronizationObjects &&) = delete;
  ~SynchronizationObjects() {
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

class Swapchain {
public:
  VkSwapchainKHR swapchain{};
  VkExtent2D extent{};
  VkFormat image_format{};
  std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> images{};
  std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> framebuffers{};

  struct SupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> present_modes{};
  };

  Swapchain() = default;
  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;
  Swapchain(Swapchain &&) = delete;
  Swapchain &operator=(Swapchain &&) = delete;
  virtual ~Swapchain() {
    destroy();
  };
  SupportDetails check_support(const VkPhysicalDevice &physical_device,
                               const VkSurfaceKHR &surface);

protected:
  void create(const VkSurfaceKHR &surface, const Queues &queues);
  void recreate(const VkSurfaceKHR &surface,
                const Queues &queues,
                SynchronizationObjects &sync_objects);

private:
  SupportDetails support_details{};
  void destroy();
  VkSurfaceFormatKHR
  pick_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats) const;
  VkPresentModeKHR pick_present_mode(
      const std::vector<VkPresentModeKHR> &available_present_modes) const;
  VkExtent2D pick_extent(GLFWwindow *window,
                        const VkSurfaceCapabilitiesKHR &capabilities) const;
  uint32_t get_image_count(const Swapchain::SupportDetails &swapchain_support) const;
};

} // namespace CE
