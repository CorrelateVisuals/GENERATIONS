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
      Lib::path("shaders/Graphics1.vert -o shaders/Graphics1Vertex.spv"),
      Lib::path("shaders/Graphics1.frag -o shaders/Graphics1Fragment.spv"),
      Lib::path("shaders/Compute1.comp -o shaders/Compute1.spv"),

      Lib::path("shaders/Graphics2.vert -o shaders/Graphics2Vertex.spv"),
      Lib::path("shaders/Graphics2.frag -o shaders/Graphics2Fragment.spv"),
  };

 public:
  void createPipelines(Resources& _resources);

  void createColorResources(Resources& _resources);
  void createDepthResources(Resources& _resources);

 private:
  VulkanMechanics& _mechanics;

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
  VkPipelineShaderStageCreateInfo getShaderStageInfo(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName,
      auto& pipeline);

  VkPipelineVertexInputStateCreateInfo getPipelineVertexInputState();
  VkPipelineColorBlendStateCreateInfo getPipelineColorBlendingState();
  VkPipelineDynamicStateCreateInfo getPipelineDynamicState();
  VkPipelineDepthStencilStateCreateInfo getPipelineDepthStencilState();
  VkPipelineInputAssemblyStateCreateInfo getPipelineInputAssemblyState(
      VkPrimitiveTopology topology);
  VkPipelineViewportStateCreateInfo getPipelineViewportState();
  VkPipelineRasterizationStateCreateInfo getPipelineRasterizationState();
  VkPipelineMultisampleStateCreateInfo getPipelineMultisampleState();
  VkPipelineLayoutCreateInfo getPipelineLayoutState(
      VkDescriptorSetLayout& descriptorSetLayout,
      VkPipelineLayoutCreateInfo& pipelineLayoutInfo);
};
