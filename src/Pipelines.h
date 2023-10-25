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
      World world;
      pipelineMap["Engine"] =
          Compute{.shaders = {"Comp"},
                  .workGroups = {
                      static_cast<uint32_t>(world.grid.size.x + 31) / 32,
                      static_cast<uint32_t>(world.grid.size.y + 31) / 32, 1}};
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
      pipelineMap["PostFX"] = Compute{
          .shaders = {"Comp"},
          .workGroups = {
              static_cast<uint32_t>(Window::get().display.width + 15) / 16,
              static_cast<uint32_t>(Window::get().display.height + 15) / 16,
              1}};
    }
  } config;

  struct Compute : public CE::PipelineLayout {
  } compute;

  struct PipelineLayouts : public CE::PipelineLayout {
  } graphics;

  struct RenderPass : public CE::RenderPass {
  } renderPass;

 public:
  void setupPipelines(Resources& _resources);

 private:
  void createPipelines(
      std::unordered_map<std::string,
                         std::variant<Configuration::Graphics,
                                      Configuration::Compute>>& pipelineMap,
      VkSampleCountFlagBits& msaaSamples);

 private:
  VulkanMechanics& _mechanics;
};
