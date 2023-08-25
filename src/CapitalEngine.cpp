#include "CapitalEngine.h"

#include <iostream>

CapitalEngine::CapitalEngine() {
  Log::console("\n", Log::Style::indentSize, "[ CAPITAL engine ]",
               "starting...\n");
  _mechanics.setupVulkan();
  _resources.createDescriptorSetLayout();
  _pipelines.createPipelines();
  _resources.createResources();
  _mechanics.createSyncObjects();
}

CapitalEngine::~CapitalEngine() {
  Log::console("\n", Log::Style::indentSize, "[ CAPITAL engine ]",
               "terminating...\n");
}

Global::~Global() {
  cleanup();
}

void CapitalEngine::mainLoop() {
  Log::console("\n", Log::Style::indentSize,
               "{ Main Loop } running ..........\n");

  while (!glfwWindowShouldClose(Window::get().window)) {
    glfwPollEvents();

    Window::get().setMouse();
    _control.time.run();

    drawFrame();

    if (glfwGetKey(Window::get().window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break;
    }
  }
  vkDeviceWaitIdle(_mechanics.mainDevice.logical);
  Log::console("\n", Log::Style::indentSize,
               "{ Main Loop } ....... terminated");
}

void CapitalEngine::drawFrame() {
  // Compute submission
  vkWaitForFences(
      _mechanics.mainDevice.logical, 1,
      &_mechanics.syncObjects
           .computeInFlightFences[_mechanics.syncObjects.currentFrame],
      VK_TRUE, UINT64_MAX);

  _resources.updateUniformBuffer(_mechanics.syncObjects.currentFrame);

  vkResetFences(
      _mechanics.mainDevice.logical, 1,
      &_mechanics.syncObjects
           .computeInFlightFences[_mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      _resources.buffers.command.compute[_mechanics.syncObjects.currentFrame],
      0);
  _resources.recordComputeCommandBuffer(
      _resources.buffers.command.compute[_mechanics.syncObjects.currentFrame]);

  VkSubmitInfo computeSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &_resources.buffers.command
                              .compute[_mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &_mechanics.syncObjects
               .computeFinishedSemaphores[_mechanics.syncObjects.currentFrame]};

  _mechanics.result(
      vkQueueSubmit, _mechanics.queues.compute, 1, &computeSubmitInfo,
      _mechanics.syncObjects
          .computeInFlightFences[_mechanics.syncObjects.currentFrame]);

  // Graphics submission
  vkWaitForFences(_mechanics.mainDevice.logical, 1,
                  &_mechanics.syncObjects
                       .inFlightFences[_mechanics.syncObjects.currentFrame],
                  VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      _mechanics.mainDevice.logical, _mechanics.swapChain.swapChain, UINT64_MAX,
      _mechanics.syncObjects
          .imageAvailableSemaphores[_mechanics.syncObjects.currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    _mechanics.recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("\n!ERROR! failed to acquire swap chain image!");
  }

  vkResetFences(_mechanics.mainDevice.logical, 1,
                &_mechanics.syncObjects
                     .inFlightFences[_mechanics.syncObjects.currentFrame]);

  vkResetCommandBuffer(
      _resources.buffers.command.graphic[_mechanics.syncObjects.currentFrame],
      0);

  _resources.recordCommandBuffer(
      _resources.buffers.command.graphic[_mechanics.syncObjects.currentFrame],
      imageIndex);

  std::vector<VkSemaphore> waitSemaphores{
      _mechanics.syncObjects
          .computeFinishedSemaphores[_mechanics.syncObjects.currentFrame],
      _mechanics.syncObjects
          .imageAvailableSemaphores[_mechanics.syncObjects.currentFrame]};
  std::vector<VkPipelineStageFlags> waitStages{
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo graphicsSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &_resources.buffers.command
                              .graphic[_mechanics.syncObjects.currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &_mechanics.syncObjects
               .renderFinishedSemaphores[_mechanics.syncObjects.currentFrame]};

  _mechanics.result(vkQueueSubmit, _mechanics.queues.graphics, 1,
                    &graphicsSubmitInfo,
                    _mechanics.syncObjects
                        .inFlightFences[_mechanics.syncObjects.currentFrame]);

  std::vector<VkSwapchainKHR> swapChains{_mechanics.swapChain.swapChain};

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &_mechanics.syncObjects
               .renderFinishedSemaphores[_mechanics.syncObjects.currentFrame],
      .swapchainCount = 1,
      .pSwapchains = swapChains.data(),
      .pImageIndices = &imageIndex};

  result = vkQueuePresentKHR(_mechanics.queues.present, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      Window::get().framebufferResized) {
    Window::get().framebufferResized = false;
    _mechanics.recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! failed to present swap chain image!");
  }

  _mechanics.syncObjects.currentFrame =
      (_mechanics.syncObjects.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Global::cleanup() {
  _mechanics.cleanupSwapChain();

  vkDestroySampler(_mechanics.mainDevice.logical,
                   _resources.image.textureSampler, nullptr);
  vkDestroyImageView(_mechanics.mainDevice.logical,
                     _resources.image.textureView, nullptr);
  vkDestroyImage(_mechanics.mainDevice.logical, _resources.image.texture,
                 nullptr);
  vkFreeMemory(_mechanics.mainDevice.logical, _resources.image.textureMemory,
               nullptr);

  vkDestroyPipeline(_mechanics.mainDevice.logical, _pipelines.graphics.pipeline,
                    nullptr);
  vkDestroyPipelineLayout(_mechanics.mainDevice.logical,
                          _pipelines.graphics.pipelineLayout, nullptr);

  vkDestroyPipeline(_mechanics.mainDevice.logical, _pipelines.compute.pipeline,
                    nullptr);
  vkDestroyPipelineLayout(_mechanics.mainDevice.logical,
                          _pipelines.compute.pipelineLayout, nullptr);

  vkDestroyRenderPass(_mechanics.mainDevice.logical,
                      _pipelines.graphics.renderPass, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(_mechanics.mainDevice.logical,

                    _resources.buffers.uniforms[i], nullptr);
    vkFreeMemory(_mechanics.mainDevice.logical,
                 _resources.buffers.uniformsMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(_mechanics.mainDevice.logical,
                          _resources.descriptor.pool, nullptr);

  vkDestroyDescriptorSetLayout(_mechanics.mainDevice.logical,
                               _resources.descriptor.setLayout, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(_mechanics.mainDevice.logical,
                    _resources.buffers.shaderStorage[i], nullptr);
    vkFreeMemory(_mechanics.mainDevice.logical,
                 _resources.buffers.shaderStorageMemory[i], nullptr);
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(_mechanics.mainDevice.logical,
                       _mechanics.syncObjects.renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(_mechanics.mainDevice.logical,
                       _mechanics.syncObjects.imageAvailableSemaphores[i],
                       nullptr);
    vkDestroySemaphore(_mechanics.mainDevice.logical,
                       _mechanics.syncObjects.computeFinishedSemaphores[i],
                       nullptr);
    vkDestroyFence(_mechanics.mainDevice.logical,
                   _mechanics.syncObjects.inFlightFences[i], nullptr);
    vkDestroyFence(_mechanics.mainDevice.logical,
                   _mechanics.syncObjects.computeInFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(_mechanics.mainDevice.logical,
                       _resources.buffers.command.pool, nullptr);

  vkDestroyDevice(_mechanics.mainDevice.logical, nullptr);

  if (ValidationLayers::isValidationEnabled()) {
    ValidationLayers::destroyDebugUtilsMessengerEXT(
        _mechanics.instance, ValidationLayers::debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(_mechanics.instance, _mechanics.surface, nullptr);
  vkDestroyInstance(_mechanics.instance, nullptr);

  glfwDestroyWindow(Window::get().window);

  glfwTerminate();
}
