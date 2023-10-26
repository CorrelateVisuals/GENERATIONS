#pragma once
#include <glm/glm.hpp>

#include "Mechanics.h"
#include "Resources.h"

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics);
  ~Pipelines();
  void setupPipelines(Resources& _resources);

  struct ComputePipelineLayout : public CE::PipelineLayout {
  } compute;
  struct GraphicsPipelineLayouts : public CE::PipelineLayout {
  } graphics;
  struct RenderPass : public CE::RenderPass {
  } renderPass;

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
              static_cast<uint32_t>(Window::get().display.width + 7) / 8,
              static_cast<uint32_t>(Window::get().display.height + 7) / 8, 1}};
    }
  } config;

 private:
  VulkanMechanics& _mechanics;
};
