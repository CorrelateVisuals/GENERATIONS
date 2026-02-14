#pragma once
#include <glm/glm.hpp>

#include "io/Library.h"
#include "platform/Window.h"
#include "world/World.h"
#include "base/VulkanPipeline.h"
#include "core/RuntimeConfig.h"

class VulkanMechanics;
class Resources;

class Pipelines {
public:
  Pipelines(VulkanMechanics &mechanics, Resources &resources);
  Pipelines(const Pipelines &) = delete;
  Pipelines &operator=(const Pipelines &) = delete;
  Pipelines(Pipelines &&) = delete;
  Pipelines &operator=(Pipelines &&) = delete;
  ~Pipelines();

  struct ComputeLayout : public CE::PipelineLayout {
    ComputeLayout(CE::DescriptorInterface &interface, CE::PushConstants &push_constant) {
      create_layout(interface.set_layout, push_constant);
    }
  };

  struct GraphicsLayout : public CE::PipelineLayout {
    GraphicsLayout(CE::DescriptorInterface &interface) {
      create_layout(interface.set_layout);
    }
  };

  struct Render : public CE::RenderPass {
    Render(CE::Swapchain &swapchain,
           const CE::Image &msaa_image,
           const VkImageView &depth_view) {
      create(msaa_image.info.samples, swapchain.image_format);
      create_framebuffers(swapchain, msaa_image.view, depth_view);
    }
  };

  struct Configuration : public CE::PipelinesConfiguration {
    static std::array<uint32_t, 3>
    default_work_groups(const std::string &pipeline_name, const Vec2UintFast16 grid_size) {
      if (pipeline_name == "Engine") {
        return {static_cast<uint32_t>(grid_size.x + 31) / 32,
                static_cast<uint32_t>(grid_size.y + 31) / 32,
                1};
      }
      if (pipeline_name == "PostFX") {
        return {static_cast<uint32_t>(Window::get().display.width + 7) / 8,
                static_cast<uint32_t>(Window::get().display.height + 7) / 8,
                1};
      }
      return {1, 1, 1};
    }

    static CE::PipelinesConfiguration::Graphics
    make_graphics(const std::string &draw_op, const std::vector<std::string> &shaders) {
      const bool is_cells_instanced =
          draw_op == "cells_instanced" || draw_op == "instanced:cells";
      if (is_cells_instanced) {
        return Graphics{.shaders = shaders,
                        .vertex_attributes = World::Cell::get_attribute_description(),
                        .vertex_bindings = World::Cell::get_binding_description()};
      }

      const bool uses_grid_geometry =
          draw_op == "grid_indexed" || draw_op == "grid_wireframe" ||
          draw_op == "indexed:grid" || draw_op == "vertices:grid";
      if (uses_grid_geometry) {
        return Graphics{.shaders = shaders,
                        .vertex_attributes = World::Grid::get_attribute_description(),
                        .vertex_bindings = World::Grid::get_binding_description()};
      }
      return Graphics{.shaders = shaders,
                      .vertex_attributes = Shape::get_attribute_description(),
                      .vertex_bindings = Shape::get_binding_description()};
    }

    Configuration(VkRenderPass &render_pass,
            const VkPipelineLayout &graphics_layout,
            const VkPipelineLayout &compute_layout,
            VkSampleCountFlagBits &msaa_samples,
            const Vec2UintFast16 grid_size) {
      const auto &runtime_definitions = CE::Runtime::get_pipeline_definitions();
      if (!runtime_definitions.empty()) {
        for (const auto &[pipeline_name, definition] : runtime_definitions) {
          if (definition.is_compute) {
            std::array<uint32_t, 3> work_groups = definition.work_groups;
            if (work_groups[0] == 0 || work_groups[1] == 0 || work_groups[2] == 0) {
              work_groups = default_work_groups(pipeline_name, grid_size);
            }
            pipeline_map.emplace(pipeline_name,
                                 Compute{.shaders = definition.shaders,
                                         .work_groups = work_groups});
            continue;
          }

          const std::string *draw_op = CE::Runtime::get_graphics_draw_op(pipeline_name);
          const std::string draw_selector = draw_op ? *draw_op : "rectangle_indexed";
          pipeline_map.emplace(pipeline_name,
                               make_graphics(draw_selector, definition.shaders));
        }
      } else {
        pipeline_map.emplace(
            "Engine",
            Compute{.shaders = {"Comp"},
              .work_groups = {static_cast<uint32_t>(grid_size.x + 31) / 32,
                      static_cast<uint32_t>(grid_size.y + 31) / 32,
                                   1}});
        pipeline_map.emplace(
            "Cells",
            Graphics{.shaders = {"Vert", "Frag"},
               .vertex_attributes = World::Cell::get_attribute_description(),
               .vertex_bindings = World::Cell::get_binding_description()});
        pipeline_map.emplace(
            "Landscape",
            Graphics{.shaders = {"Vert", "Frag"},
               .vertex_attributes = World::Grid::get_attribute_description(),
             .vertex_bindings = World::Grid::get_binding_description()});
        pipeline_map.emplace(
            "LandscapeWireFrame",
            Graphics{.shaders = {"LandscapeVert", "Tesc", "Tese", "LandscapeFrag"},
               .vertex_attributes = World::Grid::get_attribute_description(),
             .vertex_bindings = World::Grid::get_binding_description()});
        pipeline_map.emplace("Texture",
                   Graphics{.shaders = {"Vert", "Frag"},
                  .vertex_attributes = Shape::get_attribute_description(),
                  .vertex_bindings = Shape::get_binding_description()});
        pipeline_map.emplace("Water",
                   Graphics{.shaders = {"Vert", "Frag"},
                  .vertex_attributes = Shape::get_attribute_description(),
                  .vertex_bindings = Shape::get_binding_description()});
        pipeline_map.emplace(
            "PostFX",
            Compute{
                .shaders = {"Comp"},
            .work_groups = {static_cast<uint32_t>(Window::get().display.width + 7) / 8,
                    static_cast<uint32_t>(Window::get().display.height + 7) / 8,
                               1}});
      }

      compile_shaders();
      create_pipelines(render_pass, graphics_layout, compute_layout, msaa_samples);
    }
  };

  ComputeLayout compute;
  GraphicsLayout graphics;
  Render render;
  Configuration config;
};
