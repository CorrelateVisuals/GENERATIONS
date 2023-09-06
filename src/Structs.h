#pragma once
#include <vector>
#include "vulkan/vulkan.h"

namespace Structs {
struct PipelineInputAssemblyState {
  VkPipelineInputAssemblyStateCreateInfo createInfo;

  PipelineInputAssemblyState(const VkPrimitiveTopology& topology)
      : createInfo{
            .sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = topology,
            .primitiveRestartEnable = VK_FALSE} {}
};

struct PipelineVertexInputState {
  VkPipelineVertexInputStateCreateInfo createInfo;

  PipelineVertexInputState()
      : createInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr} {}
  PipelineVertexInputState(
      const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
      const std::vector<VkVertexInputAttributeDescription>&
          attributeDescriptions)
      : createInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount =
                static_cast<uint32_t>(bindingDescriptions.size()),
            .pVertexBindingDescriptions = bindingDescriptions.data(),
            .vertexAttributeDescriptionCount =
                static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data()} {}
};

struct PipelineDynamicState {
  VkPipelineDynamicStateCreateInfo createInfo;

  PipelineDynamicState()
      : createInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = new VkDynamicState[2]{
                VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}} {}

  ~PipelineDynamicState() { delete[] createInfo.pDynamicStates; }
};

struct PipelineViewportState {
  VkPipelineViewportStateCreateInfo createInfo;

  PipelineViewportState()
      : createInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1} {}
};

struct PipelineLayoutState {
  VkPipelineLayoutCreateInfo createInfo;

  PipelineLayoutState(const VkDescriptorSetLayout& descriptorSetLayout)
      : createInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                   .setLayoutCount = 1,
                   .pSetLayouts = &descriptorSetLayout} {}
  PipelineLayoutState(const VkDescriptorSetLayout& descriptorSetLayout,
                      const uint32_t& pushConstantCount,
                      const VkPushConstantRange& pushConstants)
      : createInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                   .setLayoutCount = 1,
                   .pSetLayouts = &descriptorSetLayout,
                   .pushConstantRangeCount = 1,
                   .pPushConstantRanges = &pushConstants} {}
};

struct MultisampleState {
  VkPipelineMultisampleStateCreateInfo createInfo;

  MultisampleState(uint32_t enable, const VkSampleCountFlagBits& sampleCount)
      : createInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = sampleCount,
            .sampleShadingEnable = enable,
            .minSampleShading = 1.0f} {}
};

struct PushConstantRange {
  VkPushConstantRange info;

  PushConstantRange(const VkShaderStageFlags& flags,
                    uint32_t offset,
                    uint32_t size)
      : info{.stageFlags = flags, .offset = offset, .size = size} {}
};

}  // namespace Structs
