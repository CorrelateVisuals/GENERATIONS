#include "CapitalEngine.h"
#include "Screenshot.h"
#include "Window.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace {
constexpr size_t GRAPHICS_WAIT_COUNT = 2;
constexpr uint32_t SINGLE_OBJECT_COUNT = 1;
}

CapitalEngine::CapitalEngine()
    : resources(mechanics), pipelines(mechanics, resources) {
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
    Window& mainWindow = Window::get();

    while (!glfwWindowShouldClose(mainWindow.window)) {
        mainWindow.pollInput();
    resources.world._time.run();

    drawFrame();

        if (!firstLoopScreenshotCaptured) {
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
  vkDeviceWaitIdle(mechanics.mainDevice.logical);

  Log::measureElapsedTime();
  Log::text("{ Main Loop }");
  Log::text(Log::Style::headerGuard);
}

void CapitalEngine::drawFrame() {
  // Compute submission
  vkWaitForFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects
           .computeInFlightFences[mechanics.syncObjects.currentFrame],
      VK_TRUE, UINT64_MAX);

  resources.uniform.update(resources.world, mechanics.swapchain.extent);

  vkResetFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects
           .computeInFlightFences[mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      resources.commands.compute[mechanics.syncObjects.currentFrame], 0);
  resources.commands.recordComputeCommandBuffer(resources, pipelines, mechanics.syncObjects.currentFrame);

  VkSubmitInfo computeSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers =
          &resources.commands.compute[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .computeFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  CE::VULKAN_RESULT(
      vkQueueSubmit, mechanics.queues.compute, SINGLE_OBJECT_COUNT,
      &computeSubmitInfo,
      mechanics.syncObjects
          .computeInFlightFences[mechanics.syncObjects.currentFrame]);

  // Graphics submission
  vkWaitForFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects
           .graphicsInFlightFences[mechanics.syncObjects.currentFrame],
      VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      mechanics.mainDevice.logical, mechanics.swapchain.swapchain, UINT64_MAX,
      mechanics.syncObjects
          .imageAvailableSemaphores[mechanics.syncObjects.currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    mechanics.swapchain.recreate(mechanics.initVulkan.surface, mechanics.queues,
                                 mechanics.syncObjects, pipelines, resources);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("\n!ERROR! failed to acquire swap chain image!");
  }

  vkResetFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects
           .graphicsInFlightFences[mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      resources.commands.graphics[mechanics.syncObjects.currentFrame], 0);

  resources.commands.recordGraphicsCommandBuffer(
      mechanics.swapchain, resources, pipelines,
      mechanics.syncObjects.currentFrame);

  const std::array<VkSemaphore, GRAPHICS_WAIT_COUNT> waitSemaphores{
      mechanics.syncObjects
          .computeFinishedSemaphores[mechanics.syncObjects.currentFrame],
      mechanics.syncObjects
          .imageAvailableSemaphores[mechanics.syncObjects.currentFrame]};
  const std::array<VkPipelineStageFlags, GRAPHICS_WAIT_COUNT> waitStages{
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo graphicsSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = SINGLE_OBJECT_COUNT,
      .pCommandBuffers =
          &resources.commands.graphics[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  CE::VULKAN_RESULT(
      vkQueueSubmit, mechanics.queues.graphics, SINGLE_OBJECT_COUNT,
      &graphicsSubmitInfo,
      mechanics.syncObjects
          .graphicsInFlightFences[mechanics.syncObjects.currentFrame]);

  const VkSwapchainKHR swapchain = mechanics.swapchain.swapchain;

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = SINGLE_OBJECT_COUNT,
      .pWaitSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame],
      .swapchainCount = SINGLE_OBJECT_COUNT,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
      .pResults = nullptr};

  result = vkQueuePresentKHR(mechanics.queues.present, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebufferResized) {
    Window::get().framebufferResized = false;
    mechanics.swapchain.recreate(mechanics.initVulkan.surface, mechanics.queues,
                                 mechanics.syncObjects, pipelines, resources);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }

    lastPresentedImageIndex = imageIndex;

  mechanics.syncObjects.currentFrame =
      (mechanics.syncObjects.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CapitalEngine::takeScreenshot() {
    vkQueueWaitIdle(mechanics.queues.graphics);

    std::filesystem::create_directories("screenshot");

    const std::time_t timestamp = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
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

    CE::Screenshot::capture(
            mechanics.swapchain.images[lastPresentedImageIndex].image,
            mechanics.swapchain.extent,
            mechanics.swapchain.imageFormat,
            resources.commands.pool,
            mechanics.queues.graphics,
            filename);
}
