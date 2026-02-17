#pragma once

// Runtime configuration registry for terrain/world/pipeline execution settings.
// Exists to provide one shared source of truth consumed across engine modules.

#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <string_view>

namespace CE::Runtime {

enum class DrawOpId : uint8_t {
  Unknown = 0,
  InstancedCells,
  IndexedGrid,
  IndexedGridBox,
  IndexedRectangle,
  IndexedCube,
  SkyDome,
};

DrawOpId draw_op_from_string(std::string_view draw_op);
const char *to_string(DrawOpId draw_op);

// Parses an environment-style boolean string.
//
// Returns true only for explicit truthy values:
//   "1", "true", "on" (case-insensitive)
// Any other value (including nullptr) returns false.
bool env_truthy(const char *value);

// Reads an environment variable by name and parses it via env_truthy().
//
// This centralizes env-flag semantics across the app so different call sites
// do not drift over time.
bool env_flag_enabled(const char *name);

struct PipelineExecutionPlan {
  std::vector<std::string> pre_graphics_compute{};
  std::vector<std::string> graphics{};
  std::vector<std::string> post_graphics_compute{};
};

enum class RenderStage : uint8_t {
  PreCompute = 0,
  Graphics,
  PostCompute,
};

struct RenderNode {
  RenderStage stage{RenderStage::Graphics};
  std::string pipeline{};
  DrawOpId draw_op{DrawOpId::Unknown};
};

struct RenderGraph {
  std::vector<RenderNode> nodes{};
};

struct PipelineDefinition {
  bool is_compute = false;
  std::vector<std::string> shaders{};
  std::array<uint32_t, 3> work_groups{0, 0, 0};
};

struct TerrainSettings {
  int grid_width = 50;
  int grid_height = 50;
  uint32_t alive_cells = 300;
  float cell_size = 0.5f;
  int terrain_render_subdivisions = 1;
  float terrain_box_depth = 10.0f;

  float layer1_roughness = 0.4f;
  int layer1_octaves = 10;
  float layer1_scale = 2.2f;
  float layer1_amplitude = 10.0f;
  float layer1_exponent = 2.0f;
  float layer1_frequency = 2.0f;
  float layer1_height_offset = 0.0f;

  float layer2_roughness = 1.0f;
  int layer2_octaves = 10;
  float layer2_scale = 2.2f;
  float layer2_amplitude = 1.0f;
  float layer2_exponent = 1.0f;
  float layer2_frequency = 2.0f;
  float layer2_height_offset = 0.0f;

  float blend_factor = 0.5f;
  float absolute_height = 0.0f;
};

struct WorldSettings {
  float timer_speed = 25.0f;
  float water_threshold = 0.1f;
  // Water border is established at: water_threshold + water_dead_zone_margin.
  float water_dead_zone_margin = 2.4f;
  // Width of the valid shore spawning band above the water border.
  float water_shore_band_width = 1.2f;
  // Visual highlight thickness for the water border.
  float water_border_highlight_width = 0.08f;
  std::array<float, 4> light_pos{0.0f, 20.0f, 20.0f, 0.0f};

  float zoom_speed = 0.15f;
  float panning_speed = 0.3f;
  float field_of_view = 35.0f;
  float near_clipping = 0.25f;
  float far_clipping = 800.0f;
  std::array<float, 3> camera_position{0.0f, 0.0f, 60.0f};
  float arcball_tumble_mult = 0.7f;
  float arcball_pan_mult = 0.9f;
  float arcball_dolly_mult = 0.85f;
  float arcball_pan_scalar = 0.5f;
  float arcball_zoom_scalar = 0.1f;
  float arcball_smoothing = 0.2f;
  float arcball_distance_pan_scale = 0.8f;
  float arcball_distance_zoom_scale = 0.6f;

  int cube_shape = 1;
  int rectangle_shape = 0;
  int sphere_shape = 2;
};

void set_pipeline_execution_plan(const PipelineExecutionPlan &plan);
const PipelineExecutionPlan *get_pipeline_execution_plan();
void set_render_graph(const RenderGraph &graph);
const RenderGraph *get_render_graph();

void set_pipeline_definitions(
  const std::unordered_map<std::string, PipelineDefinition> &definitions);
const std::unordered_map<std::string, PipelineDefinition> &get_pipeline_definitions();

void set_terrain_settings(const TerrainSettings &settings);
const TerrainSettings &get_terrain_settings();

void set_world_settings(const WorldSettings &settings);
const WorldSettings &get_world_settings();

void set_graphics_draw_ops(const std::unordered_map<std::string, std::string> &draw_ops);
const std::string *get_graphics_draw_op(const std::string &pipeline_name);
void set_graphics_draw_op_ids(const std::unordered_map<std::string, DrawOpId> &draw_ops);
DrawOpId get_graphics_draw_op_id(const std::string &pipeline_name);

void clear_pipeline_execution_plan();

} // namespace CE::Runtime
