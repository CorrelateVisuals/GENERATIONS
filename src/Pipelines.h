#pragma once
#include <glm/glm.hpp>

#include "Library.h"
#include "platform/Window.h"
#include "World.h"
#include "base/VulkanPipeline.h"

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics, Resources& resources);
  ~Pipelines();

  struct ComputeLayout : public CE::PipelineLayout {
    ComputeLayout(CE::DescriptorInterface& interface,
                  CE::PushConstants& push_constant) {
      create_layout(interface.set_layout, push_constant);
    }
  };

  struct GraphicsLayout : public CE::PipelineLayout {
    GraphicsLayout(CE::DescriptorInterface& interface) {
      create_layout(interface.set_layout);
    }
  };

  struct Render : public CE::RenderPass {
    Render(CE::Swapchain& swapchain,
           const CE::Image& msaa_image,
           const VkImageView& depth_view) {
      create(msaa_image.info.samples, swapchain.image_format);
      create_framebuffers(swapchain, msaa_image.view, depth_view);
    }
  };

  struct Configuration : public CE::PipelinesConfiguration {
    Configuration(VkRenderPass& render_pass,
                  const VkPipelineLayout& graphics_layout,
                  const VkPipelineLayout& compute_layout,
                  VkSampleCountFlagBits& msaa_samples,
                  const vec2_uint_fast16_t grid_size) {
        pipeline_map.emplace("Engine",
                  Compute{.shaders = {"Comp"},
                      .work_groups = {
                        static_cast<uint32_t>(grid_size.x + 31) /
                          32,
                        static_cast<uint32_t>(grid_size.y + 31) /
                          32,
                        1}});
        pipeline_map.emplace(
          "Cells",
          Graphics{.shaders = {"Vert", "Frag"},
               .vertex_attributes = World::Cell::getAttributeDescription(),
               .vertex_bindings = World::Cell::getBindingDescription()});
        pipeline_map.emplace(
          "Landscape",
          Graphics{.shaders = {"Vert", "Frag"},
               .vertex_attributes = World::Grid::getAttributeDescription(),
               .vertex_bindings = World::Grid::get_binding_description()});
        pipeline_map.emplace(
          "LandscapeWireFrame",
          Graphics{.shaders = {"LandscapeVert", "Tesc", "Tese",
                     "LandscapeFrag"},
               .vertex_attributes = World::Grid::getAttributeDescription(),
               .vertex_bindings = World::Grid::get_binding_description()});
        pipeline_map.emplace(
          "Texture",
          Graphics{.shaders = {"Vert", "Frag"},
               .vertex_attributes = Shape::get_attribute_description(),
               .vertex_bindings = Shape::get_binding_description()});
        pipeline_map.emplace(
          "Water",
          Graphics{.shaders = {"Vert", "Frag"},
               .vertex_attributes = Shape::get_attribute_description(),
               .vertex_bindings = Shape::get_binding_description()});
        pipeline_map.emplace(
          "PostFX",
          Compute{.shaders = {"Comp"},
              .work_groups = {
                static_cast<uint32_t>(Window::get().display.width + 7) /
                  8,
                static_cast<uint32_t>(Window::get().display.height + 7) /
                  8,
                1}});

      compile_shaders();
      create_pipelines(render_pass, graphics_layout, compute_layout, msaa_samples);
    }
  };

  ComputeLayout compute;
  GraphicsLayout graphics;
  Render render;
  Configuration config;
};
