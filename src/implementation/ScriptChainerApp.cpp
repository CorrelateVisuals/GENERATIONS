#include "ScriptChainerApp.h"

#include "core/Log.h"
#include "core/RuntimeConfig.h"

#include <unordered_map>

namespace {

void install_world_and_terrain_settings() {
  CE::Runtime::TerrainSettings terrain{};
  terrain.grid_width = 100;
  terrain.grid_height = 100;
  terrain.alive_cells = 2000;
  terrain.cell_size = 0.5f;
  terrain.layer1_roughness = 0.4f;
  terrain.layer1_octaves = 10;
  terrain.layer1_scale = 2.2f;
  terrain.layer1_amplitude = 10.0f;
  terrain.layer1_exponent = 2.0f;
  terrain.layer1_frequency = 2.0f;
  terrain.layer1_height_offset = 0.0f;
  terrain.layer2_roughness = 1.0f;
  terrain.layer2_octaves = 10;
  terrain.layer2_scale = 2.2f;
  terrain.layer2_amplitude = 1.0f;
  terrain.layer2_exponent = 1.0f;
  terrain.layer2_frequency = 2.0f;
  terrain.layer2_height_offset = 0.0f;
  terrain.blend_factor = 0.5f;
  terrain.absolute_height = 0.0f;
  CE::Runtime::set_terrain_settings(terrain);

  CE::Runtime::WorldSettings world{};
  world.timer_speed = 25.0f;
  world.water_threshold = 0.1f;
  world.light_pos = {0.0f, 20.0f, 20.0f, 0.0f};
  world.zoom_speed = 0.15f;
  world.panning_speed = 0.3f;
  world.field_of_view = 40.0f;
  world.near_clipping = 0.1f;
  world.far_clipping = 1000.0f;
  world.camera_position = {0.0f, 0.0f, 60.0f};
  world.arcball_tumble_mult = 0.9f;
  world.arcball_pan_mult = 0.85f;
  world.arcball_dolly_mult = 0.8f;
  world.rectangle_shape = 0;
  world.sphere_shape = 2;
  CE::Runtime::set_world_settings(world);
}

void install_pipeline_definitions() {
  std::unordered_map<std::string, CE::Runtime::PipelineDefinition> pipelines{};

  pipelines["Cells"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"CellsVert", "CellsFrag"},
  };
  pipelines["Engine"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"EngineComp"},
      .work_groups = {0, 0, 0},
  };
  pipelines["Landscape"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeFrag"},
  };
  pipelines["LandscapeWireFrame"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeWireFrameTesc", "LandscapeWireFrameTese",
                  "LandscapeFrag"},
  };
  pipelines["Texture"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"TextureVert", "TextureFrag"},
  };
  pipelines["Water"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"WaterVert", "WaterFrag"},
  };
  pipelines["PostFX"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"PostFXComp"},
      .work_groups = {0, 0, 0},
  };

  CE::Runtime::set_pipeline_definitions(pipelines);
}

void install_execution_plan() {
  CE::Runtime::PipelineExecutionPlan plan{};
  plan.graphics = {"Cells", "Landscape", "LandscapeWireFrame", "Texture", "Water"};
  plan.post_graphics_compute = {"Engine", "PostFX"};
  CE::Runtime::set_pipeline_execution_plan(plan);
}

void install_draw_bindings() {
  CE::Runtime::set_graphics_draw_ops({
      {"Cells", "instanced:cells"},
      {"Landscape", "indexed:grid"},
      {"LandscapeWireFrame", "indexed:grid"},
      {"Water", "indexed:rectangle"},
      {"Texture", "indexed:rectangle"},
  });
}

} // namespace

namespace CE::Implementation {

void ScriptChainerApp::run() {
  Log::text(Log::Style::header_guard);
  Log::text("{ C++ Graph Interface }");

  install_world_and_terrain_settings();
  install_pipeline_definitions();
  install_execution_plan();
  install_draw_bindings();

  Log::text("Runtime implementation config installed (pure C++).");
  Log::text(Log::Style::header_guard);
}

} // namespace CE::Implementation
