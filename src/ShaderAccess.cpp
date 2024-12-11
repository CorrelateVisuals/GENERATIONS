#include "ShaderAccess.h"
#include "Pipelines.h"
#include "Resources.h"

void CE::ShaderAccess::CommandResources::recordComputeCommandBuffer(
    Resources& resources,
    Pipelines& pipelines,
    const uint32_t imageIndex) {
  VkCommandBuffer commandBuffer = this->compute[imageIndex];

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelines.config.getPipelineObjectByName("Engine"));

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute.layout,
      0, 1, &resources.descriptorInterface.sets[imageIndex], 0, nullptr);

  resources.pushConstant.setData(resources.world._time.passedHours);

  vkCmdPushConstants(commandBuffer, pipelines.compute.layout,
                     resources.pushConstant.shaderStage,
                     resources.pushConstant.offset, resources.pushConstant.size,
                     resources.pushConstant.data.data());

  const std::array<uint32_t, 3>& workGroups =
      pipelines.config.getWorkGroupsByName("Engine");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[0], workGroups[2]);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

void CE::ShaderAccess::CommandResources::recordGraphicsCommandBuffer(
    CE::Swapchain& swapchain,
    Resources& resources,
    Pipelines& pipelines,
    const uint32_t imageIndex) {
  VkCommandBuffer commandBuffer = this->graphics[imageIndex];

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  CE::VULKAN_RESULT(vkBeginCommandBuffer, commandBuffer, &beginInfo);

  std::array<VkClearValue, 2> clearValues{
      VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
      VkClearValue{.depthStencil = {1.0f, 0}}};

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = pipelines.render.renderPass,
      .framebuffer = swapchain.framebuffers[imageIndex],
      .renderArea = {.offset = {0, 0}, .extent = swapchain.extent},
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()};

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport{.x = 0.0f,
                      .y = 0.0f,
                      .width = static_cast<float>(swapchain.extent.width),
                      .height = static_cast<float>(swapchain.extent.height),
                      .minDepth = 0.0f,
                      .maxDepth = 1.0f};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{.offset = {0, 0}, .extent = swapchain.extent};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.graphics.layout,
      0, 1, &resources.descriptorInterface.sets[imageIndex], 0, nullptr);

  // Pipeline 1
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Cells"));
  VkDeviceSize offsets[]{0};

  VkDeviceSize offsets0[]{0, 0};

  VkBuffer currentShaderStorageBuffer[] = {
      resources.shaderStorage.bufferIn.buffer,
      resources.shaderStorage.bufferOut.buffer};

  VkBuffer vertexBuffers0[] = {currentShaderStorageBuffer[imageIndex],
                               resources.world._cube.vertexBuffer.buffer};

  vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers0, offsets0);
  vkCmdDraw(commandBuffer,
            static_cast<uint32_t>(resources.world._cube.allVertices.size()),
            resources.world._grid.size.x * resources.world._grid.size.y, 0, 0);

  // Landscape
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Landscape"));
  VkBuffer vertexBuffers1[] = {resources.world._grid.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers1, offsets);
  vkCmdBindIndexBuffer(commandBuffer, resources.world._grid.indexBuffer.buffer,
                       0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._grid.indices.size()),
                   1, 0, 0, 0);

  //   Landscape Wireframe
  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelines.config.getPipelineObjectByName("LandscapeWireFrame"));
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._grid.indices.size()),
                   1, 0, 0, 0);

  // Pipeline 3
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Water"));
  VkBuffer vertexBuffers[] = {resources.world._rectangle.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer,
                       resources.world._rectangle.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(
      commandBuffer,
      static_cast<uint32_t>(resources.world._rectangle.indices.size()), 1, 0, 0,
      0);

  // Pipeline 4
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Texture"));
  vkCmdDrawIndexed(
      commandBuffer,
      static_cast<uint32_t>(resources.world._rectangle.indices.size()), 1, 0, 0,
      0);
  vkCmdEndRenderPass(commandBuffer);

  //       This is part of an image memory barrier (i.e., vkCmdPipelineBarrier
  //       with the VkImageMemoryBarrier parameter set)

  swapchain.images[imageIndex].transitionLayout(
      commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      /* -> */ VK_IMAGE_LAYOUT_GENERAL);

  // vkDeviceWaitIdle(_mainDevice.logical);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelines.config.getPipelineObjectByName("PostFX"));

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute.layout,
      0, 1, &resources.descriptorInterface.sets[imageIndex], 0, nullptr);

  resources.pushConstant.setData(resources.world._time.passedHours);
  vkCmdPushConstants(commandBuffer, pipelines.compute.layout,
                     resources.pushConstant.shaderStage,
                     resources.pushConstant.offset, resources.pushConstant.size,
                     resources.pushConstant.data.data());

  const std::array<uint32_t, 3>& workGroups =
      pipelines.config.getWorkGroupsByName("PostFX");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[1], workGroups[2]);

  swapchain.images[imageIndex].transitionLayout(
      commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_GENERAL,
      /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}
