#include "ImplementationSpec.h"

namespace CE::Implementation {

ImplementationSpec default_spec() {
  ImplementationSpec spec{};

  spec.terrain.grid_width = 140;
  spec.terrain.grid_height = 140;
  spec.terrain.alive_cells = 2000;
  spec.terrain.cell_size = 0.5f;
  spec.terrain.layer1_roughness = 0.4f;
  spec.terrain.layer1_octaves = 10;
  spec.terrain.layer1_scale = 2.2f;
  spec.terrain.layer1_amplitude = 10.0f;
  spec.terrain.layer1_exponent = 2.0f;
  spec.terrain.layer1_frequency = 2.0f;
  spec.terrain.layer1_height_offset = 0.0f;
  spec.terrain.layer2_roughness = 1.0f;
  spec.terrain.layer2_octaves = 10;
  spec.terrain.layer2_scale = 2.2f;
  spec.terrain.layer2_amplitude = 1.0f;
  spec.terrain.layer2_exponent = 1.0f;
  spec.terrain.layer2_frequency = 2.0f;
  spec.terrain.layer2_height_offset = 0.0f;
  spec.terrain.blend_factor = 0.5f;
  spec.terrain.absolute_height = 0.0f;

  spec.world.timer_speed = 25.0f;
  spec.world.water_threshold = 0.1f;
  spec.world.light_pos = {0.0f, 20.0f, 20.0f, 0.0f};
  spec.world.zoom_speed = 0.15f;
  spec.world.panning_speed = 0.3f;
  spec.world.field_of_view = 40.0f;
    spec.world.near_clipping = 0.6f;
    spec.world.far_clipping = 300.0f;
  spec.world.camera_position = {0.0f, 0.0f, 60.0f};
  spec.world.arcball_tumble_mult = 0.9f;
  spec.world.arcball_pan_mult = 0.85f;
  spec.world.arcball_dolly_mult = 0.8f;
  spec.world.rectangle_shape = 0;
  spec.world.sphere_shape = 2;

    spec.pipelines["Cells"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"CellsVert", "CellsFrag"},
    };
    spec.pipelines["Engine"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"EngineComp"},
      .work_groups = {0, 0, 0},
    };
    spec.pipelines["Landscape"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeFrag"},
  };
    spec.pipelines["Texture"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"TextureVert", "TextureFrag"},
  };
    spec.pipelines["PostFX"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"PostFXComp"},
      .work_groups = {0, 0, 0},
    };

    spec.execution_plan.graphics = {"Texture", "Cells", "Landscape"};
    spec.execution_plan.post_graphics_compute = {"Engine"};

  spec.draw_ops = {
      {"Texture", "indexed:rectangle"},
      {"Cells", "instanced:cells"},
      {"Landscape", "indexed:grid"},
  };

  return spec;
}

} // namespace CE::Implementation
