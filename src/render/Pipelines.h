#pragma once
#include <glm/glm.hpp>

#include "io/Library.h"
#include "platform/Window.h"
#include "world/World.h"
#include "base/VulkanPipeline.h"

class VulkanMechanics;
class Resources;

class Pipelines {
public:
  Pipelines(VulkanMechanics &mechanics, Resources &resources);
  ~Pipelines();

  struct ComputeLayout : public CE::PipelineLayout {
    ComputeLayout(CE::DescriptorInterface &interface, CE::PushConstants &pushConstant) {
      createLayout(interface.setLayout, pushConstant);
    }
  };

  struct GraphicsLayout : public CE::PipelineLayout {
    GraphicsLayout(CE::DescriptorInterface &interface) {
      createLayout(interface.setLayout);
    }
  };

  struct Render : public CE::RenderPass {
    Render(CE::Swapchain &swapchain,
           const CE::Image &msaaImage,
           const VkImageView &depthView) {
      create(msaaImage.info.samples, swapchain.image_format);
      createFramebuffers(swapchain, msaaImage.view, depthView);
    }
  };

  struct Configuration : public CE::PipelinesConfiguration {
    Configuration(VkRenderPass &renderPass,
                  const VkPipelineLayout &graphicsLayout,
                  const VkPipelineLayout &computeLayout,
                  VkSampleCountFlagBits &msaaSamples,
                  const vec2_uint_fast16_t gridSize) {
      pipelineMap.emplace(
          "Engine",
          Compute{.shaders = {"Comp"},
                  .workGroups = {static_cast<uint32_t>(gridSize.x + 31) / 32,
                                 static_cast<uint32_t>(gridSize.y + 31) / 32,
                                 1}});
      pipelineMap.emplace(
          "Cells",
          Graphics{.shaders = {"Vert", "Frag"},
                   .vertexAttributes = World::Cell::getAttributeDescription(),
                   .vertexBindings = World::Cell::getBindingDescription()});
      pipelineMap.emplace(
          "Landscape",
          Graphics{.shaders = {"Vert", "Frag"},
                   .vertexAttributes = World::Grid::getAttributeDescription(),
                   .vertexBindings = World::Grid::getBindingDescription()});
      pipelineMap.emplace(
          "LandscapeWireFrame",
          Graphics{.shaders = {"LandscapeVert", "Tesc", "Tese", "LandscapeFrag"},
                   .vertexAttributes = World::Grid::getAttributeDescription(),
                   .vertexBindings = World::Grid::getBindingDescription()});
      pipelineMap.emplace("Texture",
                          Graphics{.shaders = {"Vert", "Frag"},
                                   .vertexAttributes = Shape::getAttributeDescription(),
                                   .vertexBindings = Shape::getBindingDescription()});
      pipelineMap.emplace("Water",
                          Graphics{.shaders = {"Vert", "Frag"},
                                   .vertexAttributes = Shape::getAttributeDescription(),
                                   .vertexBindings = Shape::getBindingDescription()});
      pipelineMap.emplace(
          "PostFX",
          Compute{
              .shaders = {"Comp"},
              .workGroups = {static_cast<uint32_t>(Window::get().display.width + 7) / 8,
                             static_cast<uint32_t>(Window::get().display.height + 7) / 8,
                             1}});

      compileShaders();
      createPipelines(renderPass, graphicsLayout, computeLayout, msaaSamples);
    }
  };

  ComputeLayout compute;
  GraphicsLayout graphics;
  Render render;
  Configuration config;
};
