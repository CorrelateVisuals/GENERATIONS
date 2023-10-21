#pragma once
#include <glm/glm.hpp>

#include "Mechanics.h"
#include "Resources.h"

#include <tuple>

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics, Resources& resources);
  ~Pipelines();

  Resources& _resources;

  const std::string shaderDir = "shaders/";
  const std::unordered_map<std::string,
                           std::tuple<std::vector<std::string>, int>>
      pipelineConfig = {{"Engine", {{"Comp"}, 1}},
                        {"Cells", {{"Vert", "Frag"}, 1}},
                        {"Landscape", {{"Vert", "Tesc", "Tese", "Frag"}, 1}},
                        {"Water", {{"Vert", "Frag"}, 1}},
                        {"Texture", {{"Vert", "Frag"}, 1}},
                        {"PostFX", {{"Comp"}, 1}}};
  std::vector<VkShaderModule> shaderModules;
  std::unordered_map<std::string, CE::Pipeline> pipelineObjects;

  struct Compute : public CE::PipelineLayout {
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

  void compileShaders(const auto& pipelineConfig);
  static std::vector<char> readShaderFile(const std::string& filename);
  VkShaderModule createShaderModule(const std::vector<char>& code);
  void destroyShaderModules(std::vector<VkShaderModule>& shaderModules);
  VkPipelineShaderStageCreateInfo setShaderStage(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName);

 private:
  VulkanMechanics& _mechanics;
};
