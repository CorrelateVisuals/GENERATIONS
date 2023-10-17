#pragma once
#include <glm/glm.hpp>

#include "Mechanics.h"
#include "Resources.h"

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics, Resources& resources);
  ~Pipelines();

  Resources& _resources;

  const std::unordered_map<std::string, std::vector<std::string>> shaders = {
      {"Engine", {"Comp"}},
      {"Cells", {"Vert", "Frag"}},
      {"Landscape", {"Vert", "Tesc", "Tese", "Frag"}},
      {"Water", {"Vert", "Frag"}},
      {"Texture", {"Vert", "Frag"}},
      {"PostFX", {"Comp"}}};
  const std::string shaderDir = "shaders/";
  std::vector<VkShaderModule> shaderModules;
  std::unordered_map<std::string, CE::Pipeline> pipelineObjects;

  struct Compute {
    VkPipeline engine;
    VkPipeline postFX;
    VkPipelineLayout layout;
    const std::array<uint32_t, 3> workGroups{32, 32, 1};
  } compute;

  struct PipelineLayouts : public CE::PipelineLayout {
  } graphics;

  struct RenderPass : public CE::RenderPass {
  } renderPass;

 public:
  void setupPipelines(Resources& _resources);

 private:
  void createRenderPass(Resources& _resources);

  void createGraphicsPipeline_Layout(
      const Resources::DescriptorSets& _descriptorSets);
  void createComputePipeline_Layout(
      const Resources::DescriptorSets& _descriptorSets,
      const Resources::PushConstants& _pushConstants);

  void createGraphicsPipeline_Cells(VkSampleCountFlagBits& msaaSamples);
  void createGraphicsPipeline_Landscape(VkSampleCountFlagBits& msaaSamples);
  // void createGraphicsPipeline_LandscapeWireframe(
  //     VkSampleCountFlagBits& msaaSamples);
  void createGraphicsPipeline_Water(VkSampleCountFlagBits& msaaSamples);
  void createGraphicsPipeline_Texture(VkSampleCountFlagBits& msaaSamples);

  void createComputePipeline_Engine();
  void createComputePipeline_PostFX();

  bool hasStencilComponent(VkFormat format);

  void compileShaders();
  static std::vector<char> readShaderFile(const std::string& filename);
  VkShaderModule createShaderModule(const std::vector<char>& code);
  void destroyShaderModules(std::vector<VkShaderModule>& shaderModules);
  VkPipelineShaderStageCreateInfo setShaderStage(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName);

 private:
  VulkanMechanics& _mechanics;
};
