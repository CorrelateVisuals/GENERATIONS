#pragma once
#include "vulkan/vulkan.h"
#include <vector>

namespace Structs {
    struct PipelineInputAssemblyState {
        VkPipelineInputAssemblyStateCreateInfo createStateInfo;

        PipelineInputAssemblyState(VkPrimitiveTopology topology)
            : createStateInfo{
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                  .topology = topology,
                  .primitiveRestartEnable = VK_FALSE
            } {}
    };

    struct PipelineVertexInputState {
        VkPipelineVertexInputStateCreateInfo createStateInfo;

        PipelineVertexInputState(
            const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
            : createStateInfo{
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .vertexBindingDescriptionCount =
                      static_cast<uint32_t>(bindingDescriptions.size()),
                  .pVertexBindingDescriptions = bindingDescriptions.data(),
                  .vertexAttributeDescriptionCount =
                      static_cast<uint32_t>(attributeDescriptions.size()),
                  .pVertexAttributeDescriptions = attributeDescriptions.data()
            } {}
    };

    struct PipelineDynamicState {
        VkPipelineDynamicStateCreateInfo createStateInfo;

        PipelineDynamicState()
            : createStateInfo{
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                  .dynamicStateCount = 2,
                  .pDynamicStates = new VkDynamicState[2]{VK_DYNAMIC_STATE_VIEWPORT,
                                                          VK_DYNAMIC_STATE_SCISSOR}
            } {}

        ~PipelineDynamicState() {
            delete[] createStateInfo.pDynamicStates;
        }
    };

    struct PipelineViewportState {
        VkPipelineViewportStateCreateInfo createStateInfo;

        PipelineViewportState()
            : createStateInfo{
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                  .viewportCount = 1,
                  .scissorCount = 1
            } {}
    };

    struct PipelineLayoutState {
        VkPipelineLayoutCreateInfo createStateInfo;

        PipelineLayoutState(const VkDescriptorSetLayout& descriptorSetLayout)
            : createStateInfo{
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                  .setLayoutCount = 1,
                  .pSetLayouts = &descriptorSetLayout
            } {}
    };

    struct MultisampleState {
        VkPipelineMultisampleStateCreateInfo createStateInfo;

        MultisampleState(uint32_t enable, VkSampleCountFlagBits sampleCount)
            : createStateInfo{
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                  .rasterizationSamples = sampleCount,
                  .sampleShadingEnable = enable,
                  .minSampleShading = 1.0f
            } {}
    };
}
