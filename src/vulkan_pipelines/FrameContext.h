#pragma once

// Per-frame submission coordinator.
// Exists to sequence acquire/compute/graphics/present with correct sync ordering.

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

  VulkanMechanics &mechanics_;
  Resources &resources_;
  Pipelines &pipelines_;
};
