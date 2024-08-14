#pragma once
#include <glm/glm.hpp>

#include "BaseClasses.h"
#include "Mechanics.h"
#include "Resources.h"
#include "Geometry.h"

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics, Resources& resources);
  ~Pipelines();

  struct ComputeLayout : public CE::PipelineLayout {
    ComputeLayout(CE::PushConstants& pushConstant) {
      createLayout(CE::Descriptor::setLayout, pushConstant);
    }
  };

  struct GraphicsLayout : public CE::PipelineLayout {
    GraphicsLayout() { createLayout(CE::Descriptor::setLayout); }
  };

  struct Render : public CE::RenderPass {
    Render(CE::Swapchain& swapchain,
           const CE::Image& msaaImage,
           const VkImageView& depthView) {
      create(msaaImage.info.samples, swapchain.imageFormat);
      createFramebuffers(swapchain, msaaImage.view, depthView);
    }
  };

  struct Configuration : public CE::PipelinesConfiguration {
    Configuration(VkRenderPass& renderPass,
                  const VkPipelineLayout& graphicsLayout,
                  const VkPipelineLayout& computeLayout,
                  VkSampleCountFlagBits& msaaSamples,
                  const vec2_uint_fast16_t gridSize) {
      pipelineMap["Engine"] = Compute{
          .shaders = {"Comp"},
          .workGroups = {static_cast<uint32_t>(gridSize.x + 31) / 32,
                         static_cast<uint32_t>(gridSize.y + 31) / 32, 1}};
      pipelineMap["Cells"] =
          Graphics{.shaders = {"Vert", "Frag"},
                   .vertexAttributes = World::Cell::getAttributeDescription(),
                   .vertexBindings = World::Cell::getBindingDescription()};
      pipelineMap["Landscape"] =
          Graphics{.shaders = {"Vert", "Frag"},
                   .vertexAttributes = World::Grid::getAttributeDescription(),
                   .vertexBindings = World::Grid::getBindingDescription()};
      pipelineMap["LandscapeWireFrame"] = Graphics{
          .shaders = {"LandscapeVert", "Tesc", "Tese", "LandscapeFrag"},
          .vertexAttributes = World::Grid::getAttributeDescription(),
          .vertexBindings = World::Grid::getBindingDescription()};
      pipelineMap["Texture"] = Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = Shape::getAttributeDescription(),
          .vertexBindings = Shape::getBindingDescription()};
      pipelineMap["Water"] = Graphics{
          .shaders = {"Vert", "Frag"},
          .vertexAttributes = Shape::getAttributeDescription(),
          .vertexBindings = Shape::getBindingDescription()};
      pipelineMap["PostFX"] = Compute{
          .shaders = {"Comp"},
          .workGroups = {
              static_cast<uint32_t>(Window::get().display.width + 7) / 8,
              static_cast<uint32_t>(Window::get().display.height + 7) / 8, 1}};

      compileShaders();
      createPipelines(renderPass, graphicsLayout, computeLayout, msaaSamples);
    }
  };

  ComputeLayout compute;
  GraphicsLayout graphics;
  Render render;
  Configuration config;
};
