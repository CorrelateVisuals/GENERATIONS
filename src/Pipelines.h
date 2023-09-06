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

  const std::string shaderDir = "shaders/";
  const std::unordered_map<std::string, std::vector<std::string>> shaders = {
      {"Engine", {"Comp"}},
      {"Cells", {"Vert", "Frag"}},
      {"Tiles", {"Vert", "Frag"}},
      {"Water", {"Vert", "Frag"}}};

  struct Compute {
    VkPipeline engine;
    VkPipelineLayout pipelineLayout;
    std::vector<VkShaderModule> shaderModules;
    const std::array<uint32_t, 3> XYZ{32, 32, 1};
  } compute;

  struct Graphics {
    VkPipeline cells;
    VkPipeline tiles;
    VkPipeline water;
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
      std::string shaderName,
      auto& pipeline);

  VkPipelineLayoutCreateInfo createPipelineLayout(
      const VkDescriptorSetLayout& descriptorSetLayout,
      VkPipelineLayout& pipelineLayout);
};
