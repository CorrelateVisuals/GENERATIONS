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

  struct Graphics {
    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkPipeline pipeline2;
    std::vector<VkShaderModule> shaderModules;

    struct ConfigPipeline {
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

  struct Compute {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkShaderModule> shaderModules;
  } compute;

  std::vector<std::string> shaders{
      Lib::path("shaders/Cells.vert -o shaders/CellsVert.spv"),
      Lib::path("shaders/Cells.frag -o shaders/CellsFrag.spv"),
      Lib::path("shaders/Engine.comp -o shaders/EngineComp.spv"),

      Lib::path("shaders/Tiles.vert -o shaders/TilesVert.spv"),
      Lib::path("shaders/Tiles.frag -o shaders/TilesFrag.spv"),
  };

 public:
  void createPipelines(Resources& _resources);

  void createColorResources(Resources& _resources);
  void createDepthResources(Resources& _resources);

 private:
  VulkanMechanics& _mechanics;
  VkGraphicsPipelineCreateInfo getPipelineCreateInfo(
      Pipelines::Graphics::ConfigPipeline& pipelineConfig,
      const VkDescriptorSetLayout& descriptorSetLayout,
      const std::string& vertexShader,
      const std::string& fragmentShader);

  void createRenderPass();
  void createGraphicsPipelines(VkDescriptorSetLayout& descriptorSetLayout);
  void createComputePipeline(VkDescriptorSetLayout& descriptorSetLayout,
                             Resources::PushConstants& _pushConstants);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  bool hasStencilComponent(VkFormat format);

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
      const VkDescriptorSetLayout& descriptorSetLayout, VkPipelineLayout& pipelineLayout);
};
