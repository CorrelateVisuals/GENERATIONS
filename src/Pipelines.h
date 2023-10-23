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

  // using PipelineTuple = std::tuple<std::vector<std::string>, VkPipeline>;
  // using PipelineConfiguration = std::unordered_map<std::string,
  // PipelineTuple>;

  using PipelineTuple =
      std::tuple<std::vector<std::string>,
                 VkPipeline,
                 std::pair<Vertex, World::PipelineShaderAccessDiscription>>;
  using PipelineConfiguration = std::unordered_map<std::string, PipelineTuple>;

  World _world;
  PipelineConfiguration pipelineConfig = {
      {"Engine", {{"Comp"}, VK_NULL_HANDLE, _world.cellBindings}},
      {"Cells", {{"Vert", "Frag"}, VK_NULL_HANDLE, _world.cellBindings}},
      {"Landscape",
       {{"Vert", "Tesc", "Tese", "Frag"}, VK_NULL_HANDLE, _world.cellBindings}},
      {"Water", {{"Vert", "Frag"}, VK_NULL_HANDLE, _world.cellBindings}},
      {"Texture", {{"Vert", "Frag"}, VK_NULL_HANDLE, _world.cellBindings}},
      {"PostFX", {{"Comp"}, VK_NULL_HANDLE, _world.cellBindings}}};
  std::vector<VkShaderModule> shaderModules;

  VkPipeline& getVkPipelineObjectByName(const std::string& pipeline) {
    PipelineTuple& currentPipeline = pipelineConfig.at(pipeline);
    VkPipeline& pipelineHandle = std::get<VkPipeline>(currentPipeline);
    return pipelineHandle;
  }

  static void constructPipelinesFromShaders(
      PipelineConfiguration& pipelineConfig);

  ;
  //  std::unordered_map<std::string, CE::Pipeline> pipelineObjects;

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

  void createGraphicsPipeline_Cells(PipelineConfiguration& pipelineConfig,
                                    VkSampleCountFlagBits& msaaSamples);
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
