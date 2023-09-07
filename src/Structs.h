#pragma once
#include "vulkan/vulkan.h"

#include <vector>

namespace Structs {
    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyState(
        const VkPrimitiveTopology& topology);
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputState();
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputState(
        const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
        const std::vector<VkVertexInputAttributeDescription>&
        attributeDescriptions);
    VkPipelineDynamicStateCreateInfo pipelineDynamicState();
    VkPipelineViewportStateCreateInfo pipelineViewportState();
    VkPipelineLayoutCreateInfo pipelineLayout(
        const VkDescriptorSetLayout& descriptorSetLayout);
    VkPipelineLayoutCreateInfo pipelineLayout(
        const VkDescriptorSetLayout& descriptorSetLayout,
        const uint32_t& pushConstantCount,
        const VkPushConstantRange& pushConstants);
    VkPipelineMultisampleStateCreateInfo multisampleState(
        uint32_t enable,
        const VkSampleCountFlagBits& sampleCount);
    VkPushConstantRange pushConstantRange(const VkShaderStageFlags& flags,
        uint32_t offset,
        uint32_t size);
}  // namespace Structs
