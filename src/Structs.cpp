#include "Structs.h"

VkPipelineInputAssemblyStateCreateInfo Structs::pipelineInputAssemblyState(
    const VkPrimitiveTopology& topology) {
  VkPipelineInputAssemblyStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = topology,
      .primitiveRestartEnable = VK_FALSE};
  return createInfo;
}

VkPipelineVertexInputStateCreateInfo Structs::pipelineVertexInputState() {
  VkPipelineVertexInputStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = nullptr,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = nullptr};
  return createInfo;
}

VkPipelineVertexInputStateCreateInfo Structs::pipelineVertexInputState(
    const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>&
        attributeDescriptions) {
  VkPipelineVertexInputStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount =
          static_cast<uint32_t>(bindingDescriptions.size()),
      .pVertexBindingDescriptions = bindingDescriptions.data(),
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions = attributeDescriptions.data()};
  return createInfo;
}

VkPipelineDynamicStateCreateInfo Structs::pipelineDynamicState() {
  static std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data()};
  return createInfo;
}

VkPipelineViewportStateCreateInfo Structs::pipelineViewportState() {
  VkPipelineViewportStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};
  return createInfo;
}

VkPipelineLayoutCreateInfo Structs::pipelineLayout(
    const VkDescriptorSetLayout& descriptorSetLayout) {
  VkPipelineLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayout};
  return createInfo;
}

VkPipelineLayoutCreateInfo Structs::pipelineLayout(
    const VkDescriptorSetLayout& descriptorSetLayout,
    const uint32_t& pushConstantCount,
    const VkPushConstantRange& pushConstants) {
  VkPipelineLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstants};
  return createInfo;
}

VkPipelineMultisampleStateCreateInfo Structs::multisampleState(
    uint32_t enable,
    const VkSampleCountFlagBits& sampleCount) {
  VkPipelineMultisampleStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = sampleCount,
      .sampleShadingEnable = enable,
      .minSampleShading = 1.0f};
  return createInfo;
}

VkPushConstantRange Structs::pushConstantRange(const VkShaderStageFlags& flags,
                                               uint32_t offset,
                                               uint32_t size) {
  VkPushConstantRange info{.stageFlags = flags, .offset = offset, .size = size};
  return info;
}
