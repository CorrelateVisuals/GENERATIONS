#pragma once
#include <glm/glm.hpp>

#include "Mechanics.h"
#include "Resources.h"

#include <optional>
#include <tuple>

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics, Resources& resources);
  ~Pipelines();

  Resources& _resources;

  struct PipelineConfig : public CE::Pipelines {
    PipelineConfig() {
      pipelineMap["Engine"] =
          Config::Compute{.shaders = {"Comp"}, .workGroups = {32, 32, 1}};

      pipelineMap["Cells"] = Config::Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Cell::getAttributeDescription(),
          .vertexBindings = World::Cell::getBindingDescription()};

      pipelineMap["Landscape"] = Config::Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Landscape::getAttributeDescription(),
          .vertexBindings = World::Landscape::getBindingDescription()};

      pipelineMap["Texture"] = Config::Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Rectangle::getAttributeDescription(),
          .vertexBindings = World::Rectangle::getBindingDescription()};

      pipelineMap["Water"] = Config::Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Rectangle::getAttributeDescription(),
          .vertexBindings = World::Rectangle::getBindingDescription()};

      pipelineMap["PostFX"] =
          Config::Compute{.shaders = {"Comp"}, .workGroups = {16, 16, 1}};
    }
  };

  using PipelineTuple = std::tuple<
      std::vector<std::string>,
      VkPipeline,
      std::optional<std::vector<VkVertexInputBindingDescription> (*)()>,
      std::optional<std::vector<VkVertexInputAttributeDescription> (*)()>>;
#define PIPELINE_TUPLE_UNPACKED \
  auto& [shaderExtensions, pipeline, binding, attribute]
  using PipelineConfiguration = std::unordered_map<std::string, PipelineTuple>;

  PipelineConfiguration pipelineConfig = {
      {"Engine", {{"Comp"}, VK_NULL_HANDLE, std::nullopt, std::nullopt}},
      {"Cells",
       {{"Vert", "Frag"},
        VK_NULL_HANDLE,
        World::Cell::getBindingDescription,
        World::Cell::getAttributeDescription}},
      {"Landscape",
       {{"Vert", "Frag"},  //  "Tesc", "Tese",
        VK_NULL_HANDLE,
        World::Landscape::getBindingDescription,
        World::Landscape::getAttributeDescription}},
      {"Water",
       {{"Vert", "Frag"},
        VK_NULL_HANDLE,
        World::Rectangle::getBindingDescription,
        World::Rectangle::getAttributeDescription}},
      {"Texture",
       {{"Vert", "Frag"},
        VK_NULL_HANDLE,
        World::Rectangle::getBindingDescription,
        World::Rectangle::getAttributeDescription}},
      {"PostFX", {{"Comp"}, VK_NULL_HANDLE, std::nullopt, std::nullopt}}};
  std::vector<VkShaderModule> shaderModules;
  const std::string shaderDir = "shaders/";

  struct Compute : public CE::PipelineLayout {
    const std::array<uint32_t, 3> workGroups{32, 32, 1};
  } compute;

  struct PipelineLayouts : public CE::PipelineLayout {
  } graphics;

  struct RenderPass : public CE::RenderPass {
  } renderPass;

 public:
  void setupPipelines(Resources& _resources);
  VkPipeline& getVkPipelineObjectByName(const std::string& pipeline) {
    PipelineTuple& currentPipeline = pipelineConfig.at(pipeline);
    VkPipeline& pipelineHandle = std::get<VkPipeline>(currentPipeline);
    return pipelineHandle;
  }

 private:
  void createRenderPass(Resources& _resources);

  void createGraphicsPipeline_Layout(
      const Resources::DescriptorSets& _descriptorSets);
  void createComputePipeline_Layout(
      const Resources::DescriptorSets& _descriptorSets,
      const Resources::PushConstants& _pushConstants);

  void createPipelines(PipelineConfiguration& pipelineConfig,
                       VkSampleCountFlagBits& msaaSamples);

  bool hasStencilComponent(VkFormat format);

  void compileShaders(const auto& pipelineConfig);
  static std::vector<char> readShaderFile(const std::string& filename);
  void destroyShaderModules(std::vector<VkShaderModule>& shaderModules);
  VkPipelineShaderStageCreateInfo createShaderModules(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName);

 private:
  VulkanMechanics& _mechanics;
};
