#include "CapitalEngine.h"
#include "Window.h"

CapitalEngine::CapitalEngine()
    : pipelines(mechanics, resources), resources(mechanics, pipelines) {
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

  while (!glfwWindowShouldClose(Window::get().window)) {
    glfwPollEvents();

    vkDeviceWaitIdle(mechanics.mainDevice.logical);
    drawFrame();

    if (glfwGetKey(Window::get().window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break;
    }
  }
  vkDeviceWaitIdle(mechanics.mainDevice.logical);

  Log::measureElapsedTime();
  Log::text("{ Main Loop }");
  Log::text(Log::Style::headerGuard);
}

void CapitalEngine::drawFrame() {
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

  std::vector<VkSemaphore> waitSemaphores{
      mechanics.syncObjects
          .imageAvailableSemaphores[mechanics.syncObjects.currentFrame]};
  std::vector<VkPipelineStageFlags> waitStages{
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo graphicsSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers =
          &resources.commands.graphics[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  CE::VULKAN_RESULT(
      vkQueueSubmit, mechanics.queues.graphics, 1, &graphicsSubmitInfo,
      mechanics.syncObjects
          .graphicsInFlightFences[mechanics.syncObjects.currentFrame]);

  std::vector<VkSwapchainKHR> swapchains{mechanics.swapchain.swapchain};

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame],
      .swapchainCount = 1,
      .pSwapchains = swapchains.data(),
      .pImageIndices = &imageIndex};

  result = vkQueuePresentKHR(mechanics.queues.present, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebufferResized) {
    Window::get().framebufferResized = false;
    mechanics.swapchain.recreate(mechanics.initVulkan.surface, mechanics.queues,
                                 mechanics.syncObjects, pipelines, resources);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }

  mechanics.syncObjects.currentFrame =
      (mechanics.syncObjects.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
