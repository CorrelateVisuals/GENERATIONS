#include "FrameContext.h"

#include "vulkan_mechanics/Mechanics.h"
#include "Pipelines.h"
#include "resources/Resources.h"
#include "vulkan_base/VulkanUtils.h"
#include "control/Window.h"
#include "engine/Log.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <stdexcept>

namespace {
constexpr size_t GRAPHICS_WAIT_COUNT = 2;
constexpr uint32_t SINGLE_OBJECT_COUNT = 1;

double ms_since(const std::chrono::steady_clock::time_point &start,
                const std::chrono::steady_clock::time_point &end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

bool env_truthy_local(const char *value) {
  if (!value) {
    return false;
  }

  std::string mode(value);
  std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return mode == "1" || mode == "true" || mode == "on";
}

struct FrameProfiler {
  bool enabled = env_truthy_local(std::getenv("CE_FRAME_PROFILE"));
  uint64_t frames = 0;
  double sum_compute_wait_ms = 0.0;
  double sum_compute_work_ms = 0.0;
  double sum_graphics_wait_ms = 0.0;
  double sum_acquire_ms = 0.0;
  double sum_graphics_submit_ms = 0.0;
  double sum_present_ms = 0.0;
  double sum_frame_ms = 0.0;
  double max_frame_ms = 0.0;

  void maybe_log() const {
    if (!enabled || frames == 0 || (frames % 60) != 0) {
      return;
    }

    const double inv = 1.0 / static_cast<double>(frames);
    Log::text("{ PROF }",
              "frames",
              frames,
              "avg_frame_ms",
              sum_frame_ms * inv,
              "max_frame_ms",
              max_frame_ms,
              "avg_compute_wait_ms",
              sum_compute_wait_ms * inv,
              "avg_compute_work_ms",
              sum_compute_work_ms * inv,
              "avg_graphics_wait_ms",
              sum_graphics_wait_ms * inv,
              "avg_acquire_ms",
              sum_acquire_ms * inv,
              "avg_graphics_submit_ms",
              sum_graphics_submit_ms * inv,
              "avg_present_ms",
              sum_present_ms * inv);
  }
};

FrameProfiler g_profiler{};

struct FrameSample {
  double compute_wait_ms = 0.0;
  double compute_work_ms = 0.0;
  double graphics_wait_ms = 0.0;
  double acquire_ms = 0.0;
  double graphics_submit_ms = 0.0;
  double present_ms = 0.0;
};

thread_local FrameSample g_sample{};
}

FrameContext::FrameContext(VulkanMechanics &mechanics,
                           Resources &resources,
                           Pipelines &pipelines)
    : mechanics_(mechanics), resources_(resources), pipelines_(pipelines) {}

void FrameContext::submit_compute(const uint32_t frame_index) {
  const auto t_submit_start = std::chrono::steady_clock::now();

    // Fence wait guarantees the compute command buffer/semaphores for this frame slot
    // are no longer in-flight before we overwrite them.
  const auto t_wait_start = std::chrono::steady_clock::now();
  vkWaitForFences(mechanics_.main_device.logical_device,
                  1,
                  &mechanics_.sync_objects.compute_in_flight_fences[frame_index],
                  VK_TRUE,
                  UINT64_MAX);
  const auto t_wait_end = std::chrono::steady_clock::now();
  g_sample.compute_wait_ms = ms_since(t_wait_start, t_wait_end);

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

  const auto t_submit_end = std::chrono::steady_clock::now();
  g_sample.compute_work_ms =
      std::max(0.0, ms_since(t_submit_start, t_submit_end) - g_sample.compute_wait_ms);
}

bool FrameContext::acquire_image(const uint32_t frame_index,
                                 const std::function<void()> &recreate_swapchain,
                                 uint32_t &image_index) {
  const auto t_wait_start = std::chrono::steady_clock::now();
  // Same frame-slot rule for graphics: wait until previous use of this slot completed.
  vkWaitForFences(mechanics_.main_device.logical_device,
                  1,
                  &mechanics_.sync_objects.graphics_in_flight_fences[frame_index],
                  VK_TRUE,
                  UINT64_MAX);
  const auto t_wait_end = std::chrono::steady_clock::now();
  g_sample.graphics_wait_ms = ms_since(t_wait_start, t_wait_end);

  // Acquire provides the image-available semaphore that will gate graphics submission.
  const auto t_acquire_start = std::chrono::steady_clock::now();
  VkResult result = vkAcquireNextImageKHR(mechanics_.main_device.logical_device,
                                           mechanics_.swapchain.swapchain,
                                           UINT64_MAX,
                                           mechanics_.sync_objects.image_available_semaphores
                                               [frame_index],
                                           VK_NULL_HANDLE,
                                           &image_index);
  const auto t_acquire_end = std::chrono::steady_clock::now();
  g_sample.acquire_ms = ms_since(t_acquire_start, t_acquire_end);

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
  const auto t_submit_start = std::chrono::steady_clock::now();
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
  const auto t_submit_end = std::chrono::steady_clock::now();
  g_sample.graphics_submit_ms = ms_since(t_submit_start, t_submit_end);
}

void FrameContext::present(const uint32_t frame_index,
                           const uint32_t image_index,
                           const std::function<void()> &recreate_swapchain) {
  const auto t_present_start = std::chrono::steady_clock::now();
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
  const auto t_present_end = std::chrono::steady_clock::now();
  g_sample.present_ms = ms_since(t_present_start, t_present_end);
}

void FrameContext::draw_frame(uint32_t &last_presented_image_index,
                              uint32_t &last_submitted_frame_index,
                              const std::function<void()> &recreate_swapchain) {
  const auto t_frame_start = std::chrono::steady_clock::now();
  const uint32_t frame_index = mechanics_.sync_objects.current_frame;

  g_sample = FrameSample{};

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

  if (g_profiler.enabled) {
    const auto t_frame_end = std::chrono::steady_clock::now();
    const double frame_ms = ms_since(t_frame_start, t_frame_end);
    g_profiler.frames += 1;
    g_profiler.sum_compute_wait_ms += g_sample.compute_wait_ms;
    g_profiler.sum_compute_work_ms += g_sample.compute_work_ms;
    g_profiler.sum_graphics_wait_ms += g_sample.graphics_wait_ms;
    g_profiler.sum_acquire_ms += g_sample.acquire_ms;
    g_profiler.sum_graphics_submit_ms += g_sample.graphics_submit_ms;
    g_profiler.sum_present_ms += g_sample.present_ms;
    g_profiler.sum_frame_ms += frame_ms;
    g_profiler.max_frame_ms = std::max(g_profiler.max_frame_ms, frame_ms);
    g_profiler.maybe_log();
  }
}
