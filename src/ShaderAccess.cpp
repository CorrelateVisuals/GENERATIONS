#include "ShaderAccess.h"
#include "Pipelines.h"
#include "Resources.h" 

void CE::ShaderAccess::CommandResources::recordComputeCommandBuffer(
    Resources& resources,
    Pipelines& pipelines,
    const uint32_t imageIndex) {
    VkCommandBuffer commandBuffer = this->compute[imageIndex];

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to begin recording compute command buffer!");
    }

    CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

void CE::ShaderAccess::CommandResources::recordGraphicsCommandBuffer(
    CE::Swapchain& swapchain,
    Resources& resources,
    Pipelines& pipelines,
    const uint32_t imageIndex) {
    VkCommandBuffer commandBuffer = this->graphics[imageIndex];

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    CE::VULKAN_RESULT(vkBeginCommandBuffer, commandBuffer, &beginInfo);

    std::array<VkClearValue, 2> clearValues{
        VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        VkClearValue{.depthStencil = {1.0f, 0}} };

    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = pipelines.render.renderPass,
        .framebuffer = swapchain.framebuffers[imageIndex],
        .renderArea = {.offset = {0, 0}, .extent = swapchain.extent},
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data() };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport{ .x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<float>(swapchain.extent.width),
                        .height = static_cast<float>(swapchain.extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{ .offset = {0, 0}, .extent = swapchain.extent };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelines.config.getPipelineObjectByName("Triangle"));
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}
