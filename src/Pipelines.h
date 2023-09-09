#pragma once
#include <glm/glm.hpp>

#include "Mechanics.h"
#include "Resources.h"

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics);
  ~Pipelines();

  const std::unordered_map<std::string, std::vector<std::string>> shaders = {
      {"Engine", {"Comp"}},
      {"Cells", {"Vert", "Frag"}},
      {"Tiles", {"Vert", "Frag"}},
      {"Water", {"Vert", "Frag"}}};
  const std::string shaderDir = "shaders/";
  std::vector<VkShaderModule> shaderModules;

  struct Compute {
    VkPipeline engine;
    VkPipelineLayout layout;
    const std::array<uint32_t, 3> workGroups{32, 32, 1};
  } compute;

  struct Graphics {
    VkPipeline cells;
    VkPipeline tiles;
    VkPipeline water;
    VkPipelineLayout layout;
    VkRenderPass renderPass;

    struct Depth {
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
    } depth;

    struct MultiSampling {
      VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
    } msaa;
  } graphics;

 public:
  void createPipelines(Resources& _resources);
  void createColorResources(Resources& _resources);
  void createDepthResources(Resources& _resources);

 private:
  void createRenderPass();
  void createGraphicsPipelines(
      const VkDescriptorSetLayout& descriptorSetLayout);
  void createComputePipeline(const VkDescriptorSetLayout& descriptorSetLayout,
                             const Resources::PushConstants& _pushConstants);

  VkFormat findDepthFormat();
  bool hasStencilComponent(VkFormat format);
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);

  static std::vector<char> readShaderFile(const std::string& filename);
  VkShaderModule createShaderModule(const std::vector<char>& code);
  void destroyShaderModules(std::vector<VkShaderModule>& shaderModules);
  VkPipelineShaderStageCreateInfo setShaderStage(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName);

 private:
  VulkanMechanics& _mechanics;

  constexpr static inline VkPipelineRasterizationStateCreateInfo
      rasterizationDefault{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
          .depthClampEnable = VK_TRUE,
          .rasterizerDiscardEnable = VK_FALSE,
          .polygonMode = VK_POLYGON_MODE_FILL,
          .cullMode = VK_CULL_MODE_BACK_BIT,
          .frontFace = VK_FRONT_FACE_CLOCKWISE,
          .depthBiasEnable = VK_TRUE,
          .depthBiasConstantFactor = 0.1f,
          .depthBiasClamp = 0.01f,
          .depthBiasSlopeFactor = 0.02f,
          .lineWidth = 1.0f};

  constexpr static inline VkPipelineInputAssemblyStateCreateInfo
      inputAssemblyStateTriangleList{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
          .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
          .primitiveRestartEnable = VK_FALSE};

  constexpr static inline VkPipelineVertexInputStateCreateInfo
      vertexInputStateDefault{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .vertexBindingDescriptionCount = 0,
          .pVertexBindingDescriptions = nullptr,
          .vertexAttributeDescriptionCount = 0,
          .pVertexAttributeDescriptions = nullptr};

  constexpr static inline VkPipelineMultisampleStateCreateInfo
      multisampleStateDefault{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
          .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
          .sampleShadingEnable = VK_TRUE,
          .minSampleShading = 1.0f};

  constexpr static inline VkPipelineDepthStencilStateCreateInfo
      depthStencilStateDefault{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
          .depthTestEnable = VK_TRUE,
          .depthWriteEnable = VK_TRUE,
          .depthCompareOp = VK_COMPARE_OP_LESS,
          .depthBoundsTestEnable = VK_FALSE,
          .stencilTestEnable = VK_FALSE};

  constexpr static inline VkPipelineColorBlendAttachmentState
      colorBlendAttachmentStateFalse{
          .blendEnable = VK_FALSE,
          .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
          .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .colorBlendOp = VK_BLEND_OP_ADD,
          .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
          .alphaBlendOp = VK_BLEND_OP_ADD,
          .colorWriteMask =
              VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  constexpr static inline VkPipelineColorBlendStateCreateInfo
      colorBlendStateDefault{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .logicOpEnable = VK_FALSE,
          .logicOp = VK_LOGIC_OP_COPY,
          .attachmentCount = 1,
          .pAttachments = nullptr,
          .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

  constexpr static inline VkPipelineViewportStateCreateInfo
      viewportStateDefault{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
          .viewportCount = 1,
          .scissorCount = 1};

  constexpr static inline VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  constexpr static inline VkPipelineDynamicStateCreateInfo dynamicStateDefault{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
      .pDynamicStates = dynamicStates};

  constexpr static inline VkPipelineLayoutCreateInfo layoutDefault{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = nullptr};
};
