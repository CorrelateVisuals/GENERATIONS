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

 public:
  void createPipelines(Resources& _resources);

  void createColorResources(Resources& _resources);
  void createDepthResources(Resources& _resources);

  void destroyShaderModules(std::vector<VkShaderModule>& shaderModules);

 private:
  VulkanMechanics& _mechanics;

  void createRenderPass();
  void createGraphicsPipeline(VkDescriptorSetLayout& descriptorSetLayout);
  void createComputePipeline(VkDescriptorSetLayout& descriptorSetLayout,
                             Resources::PushConstants& _pushConstants);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  bool hasStencilComponent(VkFormat format);

  static std::vector<char> readShaderFile(const std::string& filename);
  VkShaderModule createShaderModule(const std::vector<char>& code);
  VkPipelineShaderStageCreateInfo getShaderStageInfo(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName,
      auto pipeline);

  VkPipelineVertexInputStateCreateInfo getVertexInputInfo();
  VkPipelineColorBlendStateCreateInfo getColorBlendingInfo();
  VkPipelineDynamicStateCreateInfo getDynamicStateInfo();
  VkPipelineDepthStencilStateCreateInfo getDepthStencilInfo();
};
