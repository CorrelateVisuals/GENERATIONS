#include "ShaderAccess.h"
#include "Pipelines.h"
#include "Resources.h"
#include "base/VulkanUtils.h"

#include <stdexcept>

void CE::ShaderAccess::CommandResources::record_compute_command_buffer(
    Resources &resources, Pipelines &pipelines, const uint32_t frame_index) {
  VkCommandBuffer commandBuffer = this->compute[frame_index];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelines.config.get_pipeline_object_by_name("Engine"));

  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelines.compute.layout,
                          0,
                          1,
                          &resources.descriptorInterface.sets[frame_index],
                          0,
                          nullptr);

  resources.push_constant.set_data(resources.world._time.passedHours);

  vkCmdPushConstants(commandBuffer,
                     pipelines.compute.layout,
                     resources.push_constant.shader_stage,
                     resources.push_constant.offset,
                     resources.push_constant.size,
                     resources.push_constant.data.data());

  const std::array<uint32_t, 3> &workGroups =
              pipelines.config.get_work_groups_by_name("Engine");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[0], workGroups[2]);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

void CE::ShaderAccess::CommandResources::record_graphics_command_buffer(
    CE::Swapchain &swapchain,
    Resources &resources,
    Pipelines &pipelines,
    const uint32_t frame_index,
    const uint32_t image_index) {
  VkCommandBuffer commandBuffer = this->graphics[frame_index];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  CE::VULKAN_RESULT(vkBeginCommandBuffer, commandBuffer, &beginInfo);

  std::array<VkClearValue, 2> clearValues{
      VkClearValue{.color = {{0.46f, 0.55f, 0.62f, 1.0f}}},
      VkClearValue{.depthStencil = {1.0f, 0}}};

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = pipelines.render.render_pass,
      .framebuffer = swapchain.framebuffers[image_index],
      .renderArea = {.offset = {0, 0}, .extent = swapchain.extent},
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()};

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport{.x = 0.0f,
                      .y = 0.0f,
                      .width = static_cast<float>(swapchain.extent.width),
                      .height = static_cast<float>(swapchain.extent.height),
                      .minDepth = 0.0f,
                      .maxDepth = 1.0f};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{.offset = {0, 0}, .extent = swapchain.extent};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelines.graphics.layout,
                          0,
                          1,
                          &resources.descriptorInterface.sets[frame_index],
                          0,
                          nullptr);

  const auto bindAndDrawIndexed = [&](const char *pipelineName,
                                      VkBuffer vertexBuffer,
                                      VkBuffer indexBuffer,
                                      uint32_t indexCount) {
    vkCmdBindPipeline(commandBuffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.config.get_pipeline_object_by_name(pipelineName));
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize indexedOffsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, indexedOffsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
  };

  // Pipeline 1
  vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.get_pipeline_object_by_name("Cells"));
  VkDeviceSize offsets0[]{0, 0};

  VkBuffer currentShaderStorageBuffer[] = {resources.shaderStorage.bufferIn.buffer,
                                           resources.shaderStorage.bufferOut.buffer};

  VkBuffer vertexBuffers0[] = {currentShaderStorageBuffer[frame_index],
                               resources.world._cube.vertexBuffer.buffer};

  vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers0, offsets0);
  vkCmdDraw(commandBuffer,
            static_cast<uint32_t>(resources.world._cube.allVertices.size()),
            resources.world._grid.size.x * resources.world._grid.size.y,
            0,
            0);

  // Landscape
  bindAndDrawIndexed("Landscape",
                     resources.world._grid.vertexBuffer.buffer,
                     resources.world._grid.indexBuffer.buffer,
                     static_cast<uint32_t>(resources.world._grid.indices.size()));

  //   Landscape Wireframe
  vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.get_pipeline_object_by_name("LandscapeWireFrame"));
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._grid.indices.size()),
                   1,
                   0,
                   0,
                   0);

  // Pipeline 3
  bindAndDrawIndexed("Water",
                     resources.world._rectangle.vertexBuffer.buffer,
                     resources.world._rectangle.indexBuffer.buffer,
                     static_cast<uint32_t>(resources.world._rectangle.indices.size()));

  // Pipeline 4
  bindAndDrawIndexed("Texture",
                     resources.world._rectangle.vertexBuffer.buffer,
                     resources.world._rectangle.indexBuffer.buffer,
                     static_cast<uint32_t>(resources.world._rectangle.indices.size()));
  vkCmdEndRenderPass(commandBuffer);

  //       This is part of an image memory barrier (i.e., vkCmdPipelineBarrier
  //       with the VkImageMemoryBarrier parameter set)

  swapchain.images[image_index].transition_layout(commandBuffer,
                                                  VK_FORMAT_R8G8B8A8_SRGB,
                                                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                  /* -> */ VK_IMAGE_LAYOUT_GENERAL);

  vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelines.config.get_pipeline_object_by_name("PostFX"));

  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelines.compute.layout,
                          0,
                          1,
                          &resources.descriptorInterface.sets[frame_index],
                          0,
                          nullptr);

  resources.push_constant.set_data(resources.world._time.passedHours);
  vkCmdPushConstants(commandBuffer,
                     pipelines.compute.layout,
                     resources.push_constant.shader_stage,
                     resources.push_constant.offset,
                     resources.push_constant.size,
                     resources.push_constant.data.data());

  const std::array<uint32_t, 3> &workGroups =
              pipelines.config.get_work_groups_by_name("PostFX");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[1], workGroups[2]);

  swapchain.images[image_index].transition_layout(commandBuffer,
                                                  VK_FORMAT_R8G8B8A8_SRGB,
                                                  VK_IMAGE_LAYOUT_GENERAL,
                                                  /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}
