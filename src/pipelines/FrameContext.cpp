#include "FrameContext.h"

#include "vulkan_mechanics/Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"
#include "vulkan_base/VulkanUtils.h"
#include "control/Window.h"

#include <array>
#include <stdexcept>

namespace {
constexpr size_t GRAPHICS_WAIT_COUNT = 2;
constexpr uint32_t SINGLE_OBJECT_COUNT = 1;
}

FrameContext::FrameContext(VulkanMechanics &mechanics,
                           Resources &resources,
                           Pipelines &pipelines)
    : mechanics_(mechanics), resources_(resources), pipelines_(pipelines) {}

void FrameContext::submit_compute(const uint32_t frame_index) {
    // Fence wait guarantees the compute command buffer/semaphores for this frame slot
    // are no longer in-flight before we overwrite them.
  vkWaitForFences(mechanics_.main_device.logical_device,
                  1,
                  &mechanics_.sync_objects.compute_in_flight_fences[frame_index],
                  VK_TRUE,
                  UINT64_MAX);

    // CPU writes latest world/camera parameters consumed by this frame's shaders.
  resources_.uniform.update(resources_.world, mechanics_.swapchain.extent);

  vkResetFences(mechanics_.main_device.logical_device,
                1,
                &mechanics_.sync_objects.compute_in_flight_fences[frame_index]);

  vkResetCommandBuffer(resources_.commands.compute[frame_index], 0);
  resources_.commands.record_compute_command_buffer(resources_, pipelines_, frame_index);

  VkSubmitInfo compute_submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &resources_.commands.compute[frame_index],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &mechanics_.sync_objects.compute_finished_semaphores[frame_index]};

  CE::vulkan_result(vkQueueSubmit,
                    mechanics_.queues.compute_queue,
                    SINGLE_OBJECT_COUNT,
                    &compute_submit_info,
                    mechanics_.sync_objects.compute_in_flight_fences[frame_index]);
}

bool FrameContext::acquire_image(const uint32_t frame_index,
                                 const std::function<void()> &recreate_swapchain,
                                 uint32_t &image_index) {
  // Same frame-slot rule for graphics: wait until previous use of this slot completed.
  vkWaitForFences(mechanics_.main_device.logical_device,
                  1,
                  &mechanics_.sync_objects.graphics_in_flight_fences[frame_index],
                  VK_TRUE,
                  UINT64_MAX);

  // Acquire provides the image-available semaphore that will gate graphics submission.
  VkResult result = vkAcquireNextImageKHR(mechanics_.main_device.logical_device,
                                           mechanics_.swapchain.swapchain,
                                           UINT64_MAX,
                                           mechanics_.sync_objects.image_available_semaphores
                                               [frame_index],
                                           VK_NULL_HANDLE,
                                           &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swapchain();
        return false;
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("\n!ERROR! failed to acquire swap chain image!");
  }

    return true;
}

void FrameContext::submit_graphics(const uint32_t frame_index, const uint32_t image_index) {
  vkResetFences(mechanics_.main_device.logical_device,
                1,
                &mechanics_.sync_objects.graphics_in_flight_fences[frame_index]);

  vkResetCommandBuffer(resources_.commands.graphics[frame_index], 0);
  resources_.commands.record_graphics_command_buffer(
      mechanics_.swapchain, resources_, pipelines_, frame_index, image_index);

  const std::array<VkSemaphore, GRAPHICS_WAIT_COUNT> wait_semaphores{
      mechanics_.sync_objects.compute_finished_semaphores[frame_index],
      mechanics_.sync_objects.image_available_semaphores[frame_index]};

  // Graphics waits for both:
  // 1) compute_finished (storage buffer/image data is ready),
  // 2) image_available (swapchain image can be rendered to).
  const std::array<VkPipelineStageFlags, GRAPHICS_WAIT_COUNT> wait_stages{
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo graphics_submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
      .pWaitSemaphores = wait_semaphores.data(),
      .pWaitDstStageMask = wait_stages.data(),
      .commandBufferCount = SINGLE_OBJECT_COUNT,
      .pCommandBuffers = &resources_.commands.graphics[frame_index],
      .signalSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pSignalSemaphores = &mechanics_.sync_objects.render_finished_semaphores[frame_index]};

  CE::vulkan_result(vkQueueSubmit,
                    mechanics_.queues.graphics_queue,
                    SINGLE_OBJECT_COUNT,
                    &graphics_submit_info,
                    mechanics_.sync_objects.graphics_in_flight_fences[frame_index]);
}

void FrameContext::present(const uint32_t frame_index,
                           const uint32_t image_index,
                           const std::function<void()> &recreate_swapchain) {
  const VkSwapchainKHR swapchain = mechanics_.swapchain.swapchain;

  // Present waits on render_finished so presentation only happens after rendering completes.
  VkPresentInfoKHR present_info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pWaitSemaphores = &mechanics_.sync_objects.render_finished_semaphores[frame_index],
      .swapchainCount = SINGLE_OBJECT_COUNT,
      .pSwapchains = &swapchain,
      .pImageIndices = &image_index,
      .pResults = nullptr};

  VkResult result = vkQueuePresentKHR(mechanics_.queues.present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebuffer_resized) {
    Window::get().framebuffer_resized = false;
    recreate_swapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }
}

void FrameContext::draw_frame(uint32_t &last_presented_image_index,
                              uint32_t &last_submitted_frame_index,
                              const std::function<void()> &recreate_swapchain) {
  const uint32_t frame_index = mechanics_.sync_objects.current_frame;

  submit_compute(frame_index);

  uint32_t image_index = 0;
  if (!acquire_image(frame_index, recreate_swapchain, image_index)) {
    return;
  }

  submit_graphics(frame_index, image_index);
  present(frame_index, image_index, recreate_swapchain);

  last_presented_image_index = image_index;
  last_submitted_frame_index = frame_index;

  // Move to next frame-in-flight slot (ring buffer indexing).
  mechanics_.sync_objects.current_frame =
      (mechanics_.sync_objects.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
