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

  struct Configuration : public CE::Pipelines {
    Configuration() {
      pipelineMap["Engine"] =
          Compute{.shaders = {"Comp"}, .workGroups = {32, 32, 1}};
      pipelineMap["Cells"] =
          Graphics{.shaders = {"Vert", "Frag"},
                   .vertexAttributes = World::Cell::getAttributeDescription(),
                   .vertexBindings = World::Cell::getBindingDescription()};
      pipelineMap["Landscape"] = Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Landscape::getAttributeDescription(),
          .vertexBindings = World::Landscape::getBindingDescription()};
      pipelineMap["Texture"] = Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Rectangle::getAttributeDescription(),
          .vertexBindings = World::Rectangle::getBindingDescription()};
      pipelineMap["Water"] = Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = World::Rectangle::getAttributeDescription(),
          .vertexBindings = World::Rectangle::getBindingDescription()};
      pipelineMap["PostFX"] =
          Compute{.shaders = {"Comp"}, .workGroups = {16, 16, 1}};
    }
  } config;

  struct Compute : public CE::PipelineLayout {
    const std::array<uint32_t, 3> workGroups{32, 32, 1};
  } compute;

  struct PipelineLayouts : public CE::PipelineLayout {
  } graphics;

  struct RenderPass : public CE::RenderPass {
  } renderPass;

 public:
  void setupPipelines(Resources& _resources);

  VkPipeline& getPipelineObjectByName(const std::string& name) {
    std::variant<Configuration::Graphics, Configuration::Compute>& variant =
        config.pipelineMap[name];

    if (std::holds_alternative<CE::Pipelines::Graphics>(variant)) {
      return std::get<CE::Pipelines::Graphics>(variant).pipeline;
    } else {
      return std::get<CE::Pipelines::Compute>(variant).pipeline;
    }
  }

  std::vector<std::string>& getPipelineShadersByName(
      const std::string& name,
      std::unordered_map<std::string,
                         std::variant<Configuration::Graphics,
                                      Configuration::Compute>>& pipelineMap) {
    std::variant<Configuration::Graphics, Configuration::Compute>& variant =
        pipelineMap[name];

    if (std::holds_alternative<CE::Pipelines::Graphics>(variant)) {
      return std::get<CE::Pipelines::Graphics>(variant).shaders;
    } else if (std::holds_alternative<CE::Pipelines::Compute>(variant)) {
      return std::get<CE::Pipelines::Compute>(variant).shaders;
    }
  }

  void compileShaders(
      std::unordered_map<std::string,
                         std::variant<Configuration::Graphics,
                                      Configuration::Compute>>& pipelineMap) {
    Log::text("{ GLSL }", "Compile Shaders");
    std::string systemCommand = "";
    std::string shaderExtension = "";
    std::string pipelineName = "";

    Log::text("comp  ", pipelineMap.size());

    for (const auto& entry : pipelineMap) {
      Log::text("{ GLSL }", "Compile Shaders");
      pipelineName = entry.first;
      std::vector<std::string> shaders =
          getPipelineShadersByName(pipelineName, pipelineMap);

      Log::text("comp  ", shaders.size());
      for (const auto& shader : shaders) {
        shaderExtension = Lib::upperToLowerCase(shader);
        systemCommand = Lib::path(config.shaderDir + pipelineName + "." +
                                  shaderExtension + " -o " + config.shaderDir +
                                  pipelineName + shader + ".spv");
        system(systemCommand.c_str());
      }
    }
  }

 private:
  void createRenderPass(Resources& _resources);

  void createGraphicsPipeline_Layout(
      const Resources::DescriptorSets& _descriptorSets);
  void createComputePipeline_Layout(
      const Resources::DescriptorSets& _descriptorSets,
      const Resources::PushConstants& _pushConstants);

  void createPipelines(
      std::unordered_map<std::string,
                         std::variant<Configuration::Graphics,
                                      Configuration::Compute>>& pipelineMap,
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
