#include "CapitalEngine.h"

#include <iostream>

ValidationLayers validation;
VulkanMechanics mechanics(validation);
Pipelines pipelines(mechanics);
Resources resources(mechanics);

CapitalEngine::CapitalEngine() {
  Log::text("\n");
  Log::text(Log::Style::headerGuard, "\n", Log::Style::indentSize,
            "| CAPITAL Engine");

  mechanics.setupVulkan(pipelines, resources);
  pipelines.createPipelines(resources);
  resources.createResources(pipelines);
}

CapitalEngine::~CapitalEngine() {
  Log::text(Log::Style::headerGuard, "\n", Log::Style::indentSize,
            "| CAPITAL Engine");
  Log::text(Log::Style::headerGuard, "\n");

  cleanup();
}

void CapitalEngine::mainLoop() {
  Log::text("\n");
  Log::text(Log::Style::headerGuard);
  Log::text("{ Main Loop }");

  while (!glfwWindowShouldClose(Window::get().window)) {
    glfwPollEvents();

    Window::get().setMouse();
    resources.world.time.run();

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
      resources.buffers.command.compute[mechanics.syncObjects.currentFrame], 0);
  resources.recordComputeCommandBuffer(
      resources.buffers.command.compute[mechanics.syncObjects.currentFrame],
      pipelines);

  VkSubmitInfo computeSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &resources.buffers.command
                              .compute[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .computeFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  mechanics.result(
      vkQueueSubmit, mechanics.queues.compute, 1, &computeSubmitInfo,
      mechanics.syncObjects
          .computeInFlightFences[mechanics.syncObjects.currentFrame]);

  // Graphics submission
  vkWaitForFences(
      mechanics.mainDevice.logical, 1,
      &mechanics.syncObjects.inFlightFences[mechanics.syncObjects.currentFrame],
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

  vkResetFences(mechanics.mainDevice.logical, 1,
                &mechanics.syncObjects
                     .inFlightFences[mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      resources.buffers.command.graphic[mechanics.syncObjects.currentFrame], 0);

  resources.recordCommandBuffer(
      resources.buffers.command.graphic[mechanics.syncObjects.currentFrame],
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
      .pCommandBuffers = &resources.buffers.command
                              .graphic[mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &mechanics.syncObjects
               .renderFinishedSemaphores[mechanics.syncObjects.currentFrame]};

  mechanics.result(
      vkQueueSubmit, mechanics.queues.graphics, 1, &graphicsSubmitInfo,
      mechanics.syncObjects.inFlightFences[mechanics.syncObjects.currentFrame]);

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

void CapitalEngine::cleanup() {
  mechanics.cleanupSwapChain(pipelines);

  vkDestroySampler(mechanics.mainDevice.logical, resources.image.textureSampler,
                   nullptr);
  vkDestroyImageView(mechanics.mainDevice.logical, resources.image.textureView,
                     nullptr);
  vkDestroyImage(mechanics.mainDevice.logical, resources.image.texture,
                 nullptr);
  vkFreeMemory(mechanics.mainDevice.logical, resources.image.textureMemory,
               nullptr);

  vkDestroyPipeline(mechanics.mainDevice.logical, pipelines.graphics.pipeline,
                    nullptr);
  vkDestroyPipelineLayout(mechanics.mainDevice.logical,
                          pipelines.graphics.pipelineLayout, nullptr);

  vkDestroyPipeline(mechanics.mainDevice.logical, pipelines.compute.pipeline,
                    nullptr);
  vkDestroyPipelineLayout(mechanics.mainDevice.logical,
                          pipelines.compute.pipelineLayout, nullptr);

  vkDestroyRenderPass(mechanics.mainDevice.logical,
                      pipelines.graphics.renderPass, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(mechanics.mainDevice.logical,

                    resources.buffers.uniforms[i], nullptr);
    vkFreeMemory(mechanics.mainDevice.logical,
                 resources.buffers.uniformsMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(mechanics.mainDevice.logical,
                          resources.descriptor.pool, nullptr);

  vkDestroyDescriptorSetLayout(mechanics.mainDevice.logical,
                               resources.descriptor.setLayout, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(mechanics.mainDevice.logical,
                    resources.buffers.shaderStorage[i], nullptr);
    vkFreeMemory(mechanics.mainDevice.logical,
                 resources.buffers.shaderStorageMemory[i], nullptr);
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(mechanics.mainDevice.logical,
                       mechanics.syncObjects.renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(mechanics.mainDevice.logical,
                       mechanics.syncObjects.imageAvailableSemaphores[i],
                       nullptr);
    vkDestroySemaphore(mechanics.mainDevice.logical,
                       mechanics.syncObjects.computeFinishedSemaphores[i],
                       nullptr);
    vkDestroyFence(mechanics.mainDevice.logical,
                   mechanics.syncObjects.inFlightFences[i], nullptr);
    vkDestroyFence(mechanics.mainDevice.logical,
                   mechanics.syncObjects.computeInFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(mechanics.mainDevice.logical,
                       resources.buffers.command.pool, nullptr);

  vkDestroyDevice(mechanics.mainDevice.logical, nullptr);

  if (validation.enableValidationLayers) {
    validation.DestroyDebugUtilsMessengerEXT(
        mechanics.instance, validation.debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(mechanics.instance, mechanics.surface, nullptr);
  vkDestroyInstance(mechanics.instance, nullptr);

  glfwDestroyWindow(Window::get().window);

  glfwTerminate();
}
