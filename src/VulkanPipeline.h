#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace CE {

// Forward declarations
class Swapchain;
class Image;

/**
 * @brief Push constants for passing small data to shaders
 */
struct PushConstants {
  VkShaderStageFlags shaderStage{};
  uint32_t count{};
  uint32_t offset{};
  uint32_t size{};
  std::array<uint64_t, 32> data{};

  PushConstants(VkShaderStageFlags stage,
                uint32_t dataSize,
                uint32_t dataOffset);
  virtual ~PushConstants() = default;
  void setData(const uint64_t& data);
};

/**
 * @brief Pipeline layout wrapper
 */
class PipelineLayout {
 public:
  VkPipelineLayout layout{};

  PipelineLayout() = default;
  virtual ~PipelineLayout();
  
  void createLayout(const VkDescriptorSetLayout& setLayout);
  void createLayout(const VkDescriptorSetLayout& setLayout,
                    const PushConstants& _pushConstants);
};

/**
 * @brief Render pass wrapper for Vulkan rendering
 */
class RenderPass {
 public:
  VkRenderPass renderPass{};

  RenderPass() = default;
  virtual ~RenderPass();
  
  void create(VkSampleCountFlagBits msaaImageSamples,
              VkFormat swapchainImageFormat);
  void createFramebuffers(CE::Swapchain& swapchain,
                          const VkImageView& msaaView,
                          const VkImageView& depthView) const;
};

/**
 * @brief Pipeline configuration and management
 */
class PipelinesConfiguration {
 public:
#define PIPELINE_OBJECTS \
  VkPipeline pipeline{}; \
  std::vector<std::string> shaders{};

  struct Graphics {
    PIPELINE_OBJECTS
    std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
    std::vector<VkVertexInputBindingDescription> vertexBindings{};
  };
  
  struct Compute {
    PIPELINE_OBJECTS
    std::array<uint32_t, 3> workGroups{};
  };
#undef PIPELINE_OBJECTS

  std::vector<VkShaderModule> shaderModules{};
  const std::string shaderDir = "shaders/";
  std::unordered_map<std::string, std::variant<Graphics, Compute>>
      pipelineMap{};

  PipelinesConfiguration() = default;
  virtual ~PipelinesConfiguration();
  
  void createPipelines(VkRenderPass& renderPass,
                       const VkPipelineLayout& graphicsLayout,
                       const VkPipelineLayout& computeLayout,
                       VkSampleCountFlagBits& msaaSamples);
  const std::vector<std::string>& getPipelineShadersByName(
      const std::string& name);
  VkPipeline& getPipelineObjectByName(const std::string& name);
  const std::array<uint32_t, 3>& getWorkGroupsByName(const std::string& name);

 protected:
  void compileShaders();

 private:
  bool setShaderStages(
      const std::string& pipelineName,
      std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);
  std::vector<char> readShaderFile(const std::string& filename);
  VkPipelineShaderStageCreateInfo createShaderModules(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName);
  void destroyShaderModules();
};

// ==============================================================================
// Pipeline Configuration Presets
// ==============================================================================

constexpr static inline VkPipelineRasterizationStateCreateInfo
    rasterizationCullBackBit{
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
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateMultiply{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateAdd{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateAverage{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateSubtract{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateScreen{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
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

constexpr static inline VkPipelineViewportStateCreateInfo viewportStateDefault{
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

const static inline uint32_t tessellationTopologyTriangle = 3;

constexpr static inline VkPipelineTessellationStateCreateInfo
    tessellationStateDefault{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = tessellationTopologyTriangle};

}  // namespace CE
