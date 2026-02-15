#include "ImplementationSpec.h"

#include <cstdlib>

namespace CE::Implementation {

namespace {
int parse_render_stage() {
  const char *raw = std::getenv("CE_RENDER_STAGE");
  if (!raw) {
    return 4;
  }

  char *end = nullptr;
  const long parsed = std::strtol(raw, &end, 10);
  if (end == raw || *end != '\0') {
    return 4;
  }
  if (parsed < 0) {
    return 4;
  }
  if (parsed > 5) {
    return 5;
  }
  return static_cast<int>(parsed);
}
}

ImplementationSpec default_spec() {
  ImplementationSpec spec{};
  const int render_stage = parse_render_stage();

  spec.terrain.grid_width = 140;
  spec.terrain.grid_height = 140;
  spec.terrain.alive_cells = 10000;
  spec.terrain.cell_size = 0.5f;
  spec.terrain.terrain_render_subdivisions = 2;
  spec.terrain.terrain_box_depth = 10.0f;
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
  spec.world.water_dead_zone_margin = 2.5f;
  spec.world.water_shore_band_width = 1.0f;
  spec.world.water_border_highlight_width = 0.10f;
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
  spec.world.sphere_shape = 1;

    spec.pipelines["Cells"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"CellsVert", "CellsFrag"},
    };
    spec.pipelines["CellsFollower"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"CellsFollowerVert", "CellsFrag"},
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
    spec.pipelines["LandscapeDebug"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeDebugFrag"},
  };
    spec.pipelines["LandscapeStage1"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeStage1Frag"},
  };
    spec.pipelines["LandscapeStage2"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeStage2Frag"},
  };
    spec.pipelines["TerrainBox"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"TerrainBoxSeamVert", "TerrainBoxFrag"},
  };
    spec.pipelines["Sky"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"SkyVert", "SkyFrag"},
  };
    spec.pipelines["PostFX"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"PostFXComp"},
      .work_groups = {0, 0, 0},
    };

    spec.execution_plan.pre_graphics_compute = {};
    spec.execution_plan.graphics = {"LandscapeDebug"};
    spec.execution_plan.post_graphics_compute = {};

    if (render_stage >= 1) {
      spec.execution_plan.graphics = {"LandscapeStage1"};
    }
    if (render_stage >= 2) {
      spec.execution_plan.graphics = {"LandscapeStage2"};
    }
    if (render_stage >= 3) {
      spec.execution_plan.graphics = {"Sky", "Landscape", "TerrainBox"};
    }
    if (render_stage >= 4) {
      spec.execution_plan.pre_graphics_compute = {"Engine"};
      spec.execution_plan.graphics = {"Sky", "Landscape", "TerrainBox", "CellsFollower", "Cells"};
    }

  spec.draw_ops = {
      {"Cells", "instanced:cells"},
      {"CellsFollower", "instanced:cells"},
      {"Landscape", "indexed:grid"},
      {"LandscapeDebug", "indexed:grid"},
      {"LandscapeStage1", "indexed:grid"},
      {"LandscapeStage2", "indexed:grid"},
        {"TerrainBox", "indexed:grid_box"},
      {"Sky", "sky_dome"},
  };

  return spec;
}

} // namespace CE::Implementation
