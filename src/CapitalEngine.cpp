#include "CapitalEngine.h"

#include <iostream>

CapitalEngine::CapitalEngine()
    : pipelines(mechanics, resources), resources(mechanics) {
  Log::text(Log::Style::headerGuard);
  Log::text("| CAPITAL Engine");

  mechanics.setupVulkan(pipelines, resources);
  resources.createDescriptorSetLayout(resources.descriptorSetLayoutBindings);
  resources.msaaImage.createColorResources(mechanics.swapChain.extent,
                                           mechanics.swapChain.imageFormat,
                                           resources.msaaImage.info.samples);
  resources.depthImage.createDepthResources(mechanics.swapChain.extent,
                                            CE::Image::findDepthFormat(),
                                            resources.msaaImage.info.samples);
  pipelines.setupPipelines(resources);
  resources.setupResources(pipelines);
  mechanics.createSyncObjects();
}

CapitalEngine::~CapitalEngine() {
  Log::text(Log::Style::headerGuard);
  Log::text("| CAPITAL Engine");
  Log::text(Log::Style::headerGuard);

  mechanics.cleanupSwapChain(resources);
  glfwDestroyWindow(Window::get().window);
  glfwTerminate();
}

void CapitalEngine::mainLoop() {
  Log::text(Log::Style::headerGuard);
  Log::text("{ Main Loop }");

  while (!glfwWindowShouldClose(Window::get().window)) {
    glfwPollEvents();

    Window::get().setMouse();
    resources.world.time.run();

    vkDeviceWaitIdle(mechanics.mainDevice.logical);
    drawFrame();

    if (glfwGetKey(Window::get().window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break;
    }
  }
  vkDeviceWaitIdle(mechanics.mainDevice.logical);

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

  resources.updateUniformBuffer(mechanics.syncObjects.currentFrame);

  vkResetFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects
           .computeInFlightFences[mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      resources.command.compute[mechanics.syncObjects.currentFrame], 0);
  resources.recordComputeCommandBuffer(
      resources.command.compute[mechanics.syncObjects.currentFrame], pipelines);

  VkSubmitInfo computeSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers =
          &resources.command.compute[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .computeFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  CE::vulkanResult(
      vkQueueSubmit, mechanics.queues.compute, 1, &computeSubmitInfo,
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
      mechanics.mainDevice.logical, mechanics.swapChain.swapChain, UINT64_MAX,
      mechanics.syncObjects
          .imageAvailableSemaphores[mechanics.syncObjects.currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    mechanics.recreateSwapChain(pipelines, resources);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("\n!ERROR! failed to acquire swap chain image!");
  }

  vkResetFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects
           .graphicsInFlightFences[mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      resources.command.graphics[mechanics.syncObjects.currentFrame], 0);

  resources.recordGraphicsCommandBuffer(
      resources.command.graphics[mechanics.syncObjects.currentFrame],
      imageIndex, pipelines);

  std::vector<VkSemaphore> waitSemaphores{
      mechanics.syncObjects
          .computeFinishedSemaphores[mechanics.syncObjects.currentFrame],
      mechanics.syncObjects
          .imageAvailableSemaphores[mechanics.syncObjects.currentFrame]};
  std::vector<VkPipelineStageFlags> waitStages{
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo graphicsSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers =
          &resources.command.graphics[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  CE::vulkanResult(
      vkQueueSubmit, mechanics.queues.graphics, 1, &graphicsSubmitInfo,
      mechanics.syncObjects
          .graphicsInFlightFences[mechanics.syncObjects.currentFrame]);

  std::vector<VkSwapchainKHR> swapChains{mechanics.swapChain.swapChain};

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame],
      .swapchainCount = 1,
      .pSwapchains = swapChains.data(),
      .pImageIndices = &imageIndex};

  result = vkQueuePresentKHR(mechanics.queues.present, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebufferResized) {
    Window::get().framebufferResized = false;
    mechanics.recreateSwapChain(pipelines, resources);
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }

  mechanics.syncObjects.currentFrame =
      (mechanics.syncObjects.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
