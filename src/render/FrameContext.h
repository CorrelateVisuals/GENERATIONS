#pragma once

#include <cstdint>
#include <functional>

class VulkanMechanics;
class Resources;
class Pipelines;

class FrameContext {
public:
  FrameContext(VulkanMechanics &mechanics, Resources &resources, Pipelines &pipelines);

  void draw_frame(uint32_t &last_presented_image_index,
                  uint32_t &last_submitted_frame_index,
                  const std::function<void()> &recreate_swapchain);

private:
  void submit_compute(uint32_t frame_index);
  bool acquire_image(uint32_t frame_index,
                     const std::function<void()> &recreate_swapchain,
                     uint32_t &image_index);
  void submit_graphics(uint32_t frame_index, uint32_t image_index);
  void present(uint32_t frame_index,
               uint32_t image_index,
               const std::function<void()> &recreate_swapchain);

  VkSubmitInfo create_compute_submit_info(uint32_t frame_index) const;
  VkSubmitInfo create_graphics_submit_info(uint32_t frame_index) const;
  void prepare_graphics_command(uint32_t frame_index, uint32_t image_index);
  VkPresentInfoKHR create_present_info(uint32_t frame_index, uint32_t image_index) const;
  void wait_and_reset_fence(const VkFence &fence) const;
  void prepare_compute_command(uint32_t frame_index);
  void handle_present_result(VkResult result, const std::function<void()> &recreate_swapchain) const;
  bool handle_acquire_result(VkResult result, const std::function<void()> &recreate_swapchain) const;

  VulkanMechanics &mechanics_;
  Resources &resources_;
  Pipelines &pipelines_;
};
