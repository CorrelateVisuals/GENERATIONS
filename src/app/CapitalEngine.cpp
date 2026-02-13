#include "CapitalEngine.h"
#include "core/Log.h"
#include "io/Screenshot.h"
#include "platform/Window.h"
#include "base/VulkanUtils.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace {
constexpr size_t GRAPHICS_WAIT_COUNT = 2;
constexpr uint32_t SINGLE_OBJECT_COUNT = 1;
} // namespace

CapitalEngine::CapitalEngine() : resources(mechanics), pipelines(mechanics, resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("| CAPITAL Engine");
}

CapitalEngine::~CapitalEngine() {
  Log::text(Log::Style::headerGuard);
  Log::text("| CAPITAL Engine");
  Log::text(Log::Style::headerGuard);

  glfwDestroyWindow(Window::get().window);
  glfwTerminate();
}

void CapitalEngine::mainLoop() {
  Log::text(Log::Style::headerGuard);
  Log::text("{ Main Loop }");
  Log::measureElapsedTime();

  bool firstLoopScreenshotCaptured = false;
  const auto startupScreenshotReadyAt =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  Window &mainWindow = Window::get();

  while (!glfwWindowShouldClose(mainWindow.window)) {
    mainWindow.pollInput();
    resources.world._time.run();

    drawFrame();

    if (!firstLoopScreenshotCaptured &&
        std::chrono::steady_clock::now() >= startupScreenshotReadyAt) {
      firstLoopScreenshotCaptured = true;
      Log::text("{ >>> }", "Main loop startup screenshot capture");
      takeScreenshot();
    }

    if (mainWindow.consumeScreenshotPressed()) {
      Log::text("{ >>> }", "F12 pressed - capturing screenshot");
      takeScreenshot();
    }

    if (mainWindow.isEscapePressed()) {
      break;
    }
  }
  vkDeviceWaitIdle(mechanics.mainDevice.logical_device);

  Log::measureElapsedTime();
  Log::text(Log::Style::headerGuard);
}

void CapitalEngine::drawFrame() {
  const uint32_t frameIndex = mechanics.syncObjects.current_frame;

  // Compute submission
  vkWaitForFences(mechanics.mainDevice.logical_device,
                  1,
                  &mechanics.syncObjects.compute_in_flight_fences[frameIndex],
                  VK_TRUE,
                  UINT64_MAX);

  resources.uniform.update(resources.world, mechanics.swapchain.extent);

  vkResetFences(mechanics.mainDevice.logical_device,
                1,
                &mechanics.syncObjects.compute_in_flight_fences[frameIndex]);

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
      .pSignalSemaphores = &mechanics.syncObjects.compute_finished_semaphores[frameIndex]};

  CE::VULKAN_RESULT(vkQueueSubmit,
                    mechanics.queues.compute_queue,
                    SINGLE_OBJECT_COUNT,
                    &computeSubmitInfo,
                    mechanics.syncObjects.compute_in_flight_fences[frameIndex]);

  // Graphics submission
  vkWaitForFences(mechanics.mainDevice.logical_device,
                  1,
                  &mechanics.syncObjects.graphics_in_flight_fences[frameIndex],
                  VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(mechanics.mainDevice.logical_device,
                            mechanics.swapchain.swapchain,
                            UINT64_MAX,
                            mechanics.syncObjects.image_available_semaphores[frameIndex],
                            VK_NULL_HANDLE,
                            &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    mechanics.swapchain.recreate(mechanics.initVulkan.surface,
                                 mechanics.queues,
                                 mechanics.syncObjects,
                                 pipelines,
                                 resources);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("\n!ERROR! failed to acquire swap chain image!");
  }

  vkResetFences(mechanics.mainDevice.logical_device,
                1,
                &mechanics.syncObjects.graphics_in_flight_fences[frameIndex]);

  vkResetCommandBuffer(resources.commands.graphics[frameIndex], 0);

    resources.commands.record_graphics_command_buffer(
      mechanics.swapchain, resources, pipelines, frameIndex, imageIndex);

  const std::array<VkSemaphore, GRAPHICS_WAIT_COUNT> waitSemaphores{
      mechanics.syncObjects.compute_finished_semaphores[frameIndex],
      mechanics.syncObjects.image_available_semaphores[frameIndex]};
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
      .pSignalSemaphores = &mechanics.syncObjects.render_finished_semaphores[frameIndex]};

  CE::VULKAN_RESULT(vkQueueSubmit,
                    mechanics.queues.graphics_queue,
                    SINGLE_OBJECT_COUNT,
                    &graphicsSubmitInfo,
                    mechanics.syncObjects.graphics_in_flight_fences[frameIndex]);

  const VkSwapchainKHR swapchain = mechanics.swapchain.swapchain;

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pWaitSemaphores = &mechanics.syncObjects.render_finished_semaphores[frameIndex],
      .swapchainCount = SINGLE_OBJECT_COUNT,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
      .pResults = nullptr};

  result = vkQueuePresentKHR(mechanics.queues.present_queue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebufferResized) {
    Window::get().framebufferResized = false;
    mechanics.swapchain.recreate(mechanics.initVulkan.surface,
                                 mechanics.queues,
                                 mechanics.syncObjects,
                                 pipelines,
                                 resources);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }

  lastPresentedImageIndex = imageIndex;

    mechanics.syncObjects.current_frame =
      (mechanics.syncObjects.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CapitalEngine::takeScreenshot() {
  vkQueueWaitIdle(mechanics.queues.graphics_queue);

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

  CE::Screenshot::capture(mechanics.swapchain.images[lastPresentedImageIndex].image,
                          mechanics.swapchain.extent,
                          mechanics.swapchain.image_format,
                          resources.commands.pool,
                          mechanics.queues.graphics_queue,
                          filename);
}
