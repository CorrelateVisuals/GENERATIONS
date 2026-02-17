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

constexpr int kDefaultGridWidth = 10;
constexpr int kDefaultGridHeight = 10;
constexpr uint32_t kDefaultAliveCells = 50;

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
  int grid_width{};
  int grid_height{};
  uint32_t alive_cells{};
  float cell_size{};
  int terrain_render_subdivisions{};
  float terrain_box_depth{};

  float layer1_roughness{};
  int layer1_octaves{};
  float layer1_scale{};
  float layer1_amplitude{};
  float layer1_exponent{};
  float layer1_frequency{};
  float layer1_height_offset{};

  float layer2_roughness{};
  int layer2_octaves{};
  float layer2_scale{};
  float layer2_amplitude{};
  float layer2_exponent{};
  float layer2_frequency{};
  float layer2_height_offset{};

  float blend_factor{};
  float absolute_height{};
};

struct WorldSettings {
  float timer_speed{};
  float water_threshold{};
  // Water border is established at: water_threshold + water_dead_zone_margin.
  float water_dead_zone_margin{};
  // Width of the valid shore spawning band above the water border.
  float water_shore_band_width{};
  // Visual highlight thickness for the water border.
  float water_border_highlight_width{};
  std::array<float, 4> light_pos{};

  float zoom_speed{};
  float panning_speed{};
  float field_of_view{};
  float near_clipping{};
  float far_clipping{};
  std::array<float, 3> camera_position{};
  float arcball_tumble_mult{};
  float arcball_pan_mult{};
  float arcball_dolly_mult{};
  float arcball_pan_scalar{};
  float arcball_zoom_scalar{};
  float arcball_smoothing{};
  float arcball_distance_pan_scale{};
  float arcball_distance_zoom_scale{};

  int cube_shape{};
  int rectangle_shape{};
  int sphere_shape{};
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
