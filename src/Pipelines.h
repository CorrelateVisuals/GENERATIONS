#pragma once
#include <glm/glm.hpp>
#include <map>

#include "Mechanics.h"
#include "Resources.h"

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics);
  ~Pipelines();

  std::map<std::string, std::vector<std::string>> shaders = {
      {"Engine", {"Comp"}},
      {"Cells", {"Vert", "Frag"}},
      {"Tiles", {"Vert", "Frag"}}};

  struct Compute {
    VkPipeline engine;
    VkPipelineLayout pipelineLayout;
    std::vector<VkShaderModule> shaderModules;
  } compute;

  struct Graphics {
    VkPipeline cells;
    VkPipeline tiles;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    std::vector<VkShaderModule> shaderModules;

    struct Depth {
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
    } depth;

    struct MultiSampling {
      VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
      VkImage colorImage;
      VkDeviceMemory colorImageMemory;
      VkImageView colorImageView;
    } msaa;
  } graphics;

 public:
  void createPipelines(Resources& _resources);
  void createColorResources(Resources& _resources);
  void createDepthResources(Resources& _resources);

 private:
  VulkanMechanics& _mechanics;

  struct GraphicsConfig {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineColorBlendStateCreateInfo colorBlendingState;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineLayoutCreateInfo pipelineLayoutState;
  };

 private:
  void createRenderPass();
  VkGraphicsPipelineCreateInfo setGraphicsConfig(
      Pipelines::GraphicsConfig& pipelineConfig,
      const VkDescriptorSetLayout& descriptorSetLayout,
      const std::string& vertexShader,
      const std::string& fragmentShader);
  void createGraphicsPipelines(VkDescriptorSetLayout& descriptorSetLayout);
  void createComputePipeline(VkDescriptorSetLayout& descriptorSetLayout,
                             Resources::PushConstants& _pushConstants);

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
      std::string shaderName,
      auto& pipeline);

  VkPipelineVertexInputStateCreateInfo getVertexInputState();
  VkPipelineColorBlendStateCreateInfo getColorBlendingState();
  VkPipelineDynamicStateCreateInfo getDynamicState();
  VkPipelineDepthStencilStateCreateInfo getDepthStencilState();
  VkPipelineInputAssemblyStateCreateInfo getInputAssemblyState(
      VkPrimitiveTopology topology);
  VkPipelineViewportStateCreateInfo getViewportState();
  VkPipelineRasterizationStateCreateInfo getRasterizationState();
  VkPipelineMultisampleStateCreateInfo getMultisampleState();
  VkPipelineLayoutCreateInfo setLayoutState(
      const VkDescriptorSetLayout& descriptorSetLayout,
      VkPipelineLayout& pipelineLayout);
};
