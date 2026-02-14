#include "CapitalEngine.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "io/Screenshot.h"
#include "platform/Window.h"
#include "base/VulkanUtils.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace {
constexpr size_t GRAPHICS_WAIT_COUNT = 2;
constexpr uint32_t SINGLE_OBJECT_COUNT = 1;
} // namespace

CapitalEngine::CapitalEngine() : resources(mechanics), pipelines(mechanics, resources) {
  Log::text(Log::Style::header_guard);
  Log::text("| CAPITAL Engine");
}

CapitalEngine::~CapitalEngine() {
  if (mechanics.main_device.logical_device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(mechanics.main_device.logical_device);
  }

  Log::text(Log::Style::header_guard);
  Log::text("| CAPITAL Engine");
  Log::text(Log::Style::header_guard);
}

void CapitalEngine::main_loop() {
  Log::text(Log::Style::header_guard);
  Log::text("{ Main Loop }");
  Log::measure_elapsed_time();

  const bool startup_screenshot_enabled =
      CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT");
  bool first_loop_screenshot_captured = !startup_screenshot_enabled;
  const auto startup_screenshot_ready_at =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  Window &main_window = Window::get();

  while (!glfwWindowShouldClose(main_window.window)) {
    main_window.poll_input();
    resources.world._time.run();

    draw_frame();

    if (!first_loop_screenshot_captured &&
        std::chrono::steady_clock::now() >= startup_screenshot_ready_at) {
      first_loop_screenshot_captured = true;
      Log::text("{ >>> }", "Main loop startup screenshot capture");
      take_screenshot();
    }

    if (main_window.consume_screenshot_pressed()) {
      Log::text("{ >>> }", "F12 pressed - capturing screenshot");
      take_screenshot();
    }

    if (main_window.is_escape_pressed()) {
      break;
    }
  }
  vkDeviceWaitIdle(mechanics.main_device.logical_device);

  Log::measure_elapsed_time();
  Log::text(Log::Style::header_guard);
}

void CapitalEngine::draw_frame() {
  const uint32_t frameIndex = mechanics.sync_objects.current_frame;

  // Compute submission
  vkWaitForFences(mechanics.main_device.logical_device,
                  1,
                  &mechanics.sync_objects.compute_in_flight_fences[frameIndex],
                  VK_TRUE,
                  UINT64_MAX);

  resources.uniform.update(resources.world, mechanics.swapchain.extent);

  vkResetFences(mechanics.main_device.logical_device,
                1,
                &mechanics.sync_objects.compute_in_flight_fences[frameIndex]);

  vkResetCommandBuffer(resources.commands.compute[frameIndex], 0);
  resources.commands.record_compute_command_buffer(resources, pipelines, frameIndex);

  VkSubmitInfo computeSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &resources.commands.compute[frameIndex],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &mechanics.sync_objects.compute_finished_semaphores[frameIndex]};

  CE::vulkan_result(vkQueueSubmit,
                    mechanics.queues.compute_queue,
                    SINGLE_OBJECT_COUNT,
                    &computeSubmitInfo,
                    mechanics.sync_objects.compute_in_flight_fences[frameIndex]);

  // Graphics submission
  vkWaitForFences(mechanics.main_device.logical_device,
                  1,
                  &mechanics.sync_objects.graphics_in_flight_fences[frameIndex],
                  VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(mechanics.main_device.logical_device,
                            mechanics.swapchain.swapchain,
                            UINT64_MAX,
                mechanics.sync_objects.image_available_semaphores[frameIndex],
                            VK_NULL_HANDLE,
                            &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                 mechanics.queues,
                   mechanics.sync_objects,
                                 pipelines,
                                 resources);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("\n!ERROR! failed to acquire swap chain image!");
  }

  vkResetFences(mechanics.main_device.logical_device,
                1,
                &mechanics.sync_objects.graphics_in_flight_fences[frameIndex]);

  vkResetCommandBuffer(resources.commands.graphics[frameIndex], 0);

    resources.commands.record_graphics_command_buffer(
      mechanics.swapchain, resources, pipelines, frameIndex, imageIndex);

  const std::array<VkSemaphore, GRAPHICS_WAIT_COUNT> waitSemaphores{
      mechanics.sync_objects.compute_finished_semaphores[frameIndex],
      mechanics.sync_objects.image_available_semaphores[frameIndex]};
  const std::array<VkPipelineStageFlags, GRAPHICS_WAIT_COUNT> waitStages{
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo graphicsSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = SINGLE_OBJECT_COUNT,
      .pCommandBuffers = &resources.commands.graphics[frameIndex],
      .signalSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pSignalSemaphores = &mechanics.sync_objects.render_finished_semaphores[frameIndex]};

  CE::vulkan_result(vkQueueSubmit,
                    mechanics.queues.graphics_queue,
                    SINGLE_OBJECT_COUNT,
                    &graphicsSubmitInfo,
                    mechanics.sync_objects.graphics_in_flight_fences[frameIndex]);

  const VkSwapchainKHR swapchain = mechanics.swapchain.swapchain;

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pWaitSemaphores = &mechanics.sync_objects.render_finished_semaphores[frameIndex],
      .swapchainCount = SINGLE_OBJECT_COUNT,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
      .pResults = nullptr};

  result = vkQueuePresentKHR(mechanics.queues.present_queue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebuffer_resized) {
    Window::get().framebuffer_resized = false;
    mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                 mechanics.queues,
                   mechanics.sync_objects,
                                 pipelines,
                                 resources);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }

  last_presented_image_index = imageIndex;
  last_submitted_frame_index = frameIndex;

    mechanics.sync_objects.current_frame =
      (mechanics.sync_objects.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CapitalEngine::take_screenshot() {
  vkWaitForFences(mechanics.main_device.logical_device,
                  1,
                  &mechanics.sync_objects.graphics_in_flight_fences[last_submitted_frame_index],
                  VK_TRUE,
                  UINT64_MAX);

  std::filesystem::create_directories("screenshot");

  const std::time_t timestamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm timeInfo{};
#ifdef __linux__
  localtime_r(&timestamp, &timeInfo);
#else
  localtime_s(&timeInfo, &timestamp);
#endif

  std::ostringstream nameBuilder;
  nameBuilder << "screenshot/screenshot_" << std::put_time(&timeInfo, "%Y%m%d_%H%M%S")
              << ".png";
  const std::string filename = nameBuilder.str();

  CE::Screenshot::capture(mechanics.swapchain.images[last_presented_image_index].image,
                          mechanics.swapchain.extent,
                          mechanics.swapchain.image_format,
                          resources.commands.pool,
                          mechanics.queues.graphics_queue,
                          filename);
}
