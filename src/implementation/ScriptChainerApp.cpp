#include "ScriptChainerApp.h"

#include "GraphExecutionPlan.h"
#include "PythonGraphScript.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"

#include <array>
#include <charconv>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace {

float parse_float_setting(const std::unordered_map<std::string, std::string> &settings,
              const char *key,
              const float fallback) {
  const auto it = settings.find(key);
  if (it == settings.end()) {
  return fallback;
  }
  return std::stof(it->second);
}

int parse_int_setting(const std::unordered_map<std::string, std::string> &settings,
            const char *key,
            const int fallback) {
  const auto it = settings.find(key);
  if (it == settings.end()) {
  return fallback;
  }
  return std::stoi(it->second);
}

uint32_t parse_uint_setting(const std::unordered_map<std::string, std::string> &settings,
              const char *key,
              const uint32_t fallback) {
  const auto it = settings.find(key);
  if (it == settings.end()) {
  return fallback;
  }
  return static_cast<uint32_t>(std::stoul(it->second));
}

template <size_t N>
std::array<float, N> parse_float_array_setting(
  const std::unordered_map<std::string, std::string> &settings,
  const char *key,
  const std::array<float, N> &fallback) {
  const auto it = settings.find(key);
  if (it == settings.end()) {
  return fallback;
  }

  std::array<float, N> parsed{};
  std::string_view text(it->second);
  size_t start = 0;
  for (size_t i = 0; i < N; i++) {
  const size_t end = text.find(',', start);
  const std::string_view token =
    (end == std::string_view::npos) ? text.substr(start) : text.substr(start, end - start);
  if (token.empty()) {
    return fallback;
  }
  parsed[i] = std::stof(std::string(token));
  if (end == std::string_view::npos) {
    if (i + 1 != N) {
    return fallback;
    }
    start = text.size();
    continue;
  }
  start = end + 1;
  }
  if (start < text.size()) {
  return fallback;
  }
  return parsed;
}

void apply_graph_runtime_settings(const CE::Implementation::ShaderGraph &graph) {
  const auto &settings = graph.settings();

  CE::Runtime::TerrainSettings terrain = CE::Runtime::get_terrain_settings();
  terrain.grid_width = parse_int_setting(settings, "terrain.grid_width", terrain.grid_width);
  terrain.grid_height = parse_int_setting(settings, "terrain.grid_height", terrain.grid_height);
  terrain.alive_cells =
    parse_uint_setting(settings, "terrain.alive_cells", terrain.alive_cells);
  terrain.cell_size = parse_float_setting(settings, "terrain.cell_size", terrain.cell_size);
  terrain.layer1_roughness =
    parse_float_setting(settings, "terrain.layer1.roughness", terrain.layer1_roughness);
  terrain.layer1_octaves =
    parse_int_setting(settings, "terrain.layer1.octaves", terrain.layer1_octaves);
  terrain.layer1_scale =
    parse_float_setting(settings, "terrain.layer1.scale", terrain.layer1_scale);
  terrain.layer1_amplitude =
    parse_float_setting(settings, "terrain.layer1.amplitude", terrain.layer1_amplitude);
  terrain.layer1_exponent =
    parse_float_setting(settings, "terrain.layer1.exponent", terrain.layer1_exponent);
  terrain.layer1_frequency =
    parse_float_setting(settings, "terrain.layer1.frequency", terrain.layer1_frequency);
  terrain.layer1_height_offset = parse_float_setting(
    settings, "terrain.layer1.height_offset", terrain.layer1_height_offset);

  terrain.layer2_roughness =
    parse_float_setting(settings, "terrain.layer2.roughness", terrain.layer2_roughness);
  terrain.layer2_octaves =
    parse_int_setting(settings, "terrain.layer2.octaves", terrain.layer2_octaves);
  terrain.layer2_scale =
    parse_float_setting(settings, "terrain.layer2.scale", terrain.layer2_scale);
  terrain.layer2_amplitude =
    parse_float_setting(settings, "terrain.layer2.amplitude", terrain.layer2_amplitude);
  terrain.layer2_exponent =
    parse_float_setting(settings, "terrain.layer2.exponent", terrain.layer2_exponent);
  terrain.layer2_frequency =
    parse_float_setting(settings, "terrain.layer2.frequency", terrain.layer2_frequency);
  terrain.layer2_height_offset = parse_float_setting(
    settings, "terrain.layer2.height_offset", terrain.layer2_height_offset);

  terrain.blend_factor =
    parse_float_setting(settings, "terrain.blend_factor", terrain.blend_factor);
  terrain.absolute_height =
    parse_float_setting(settings, "terrain.absolute_height", terrain.absolute_height);

  CE::Runtime::set_terrain_settings(terrain);

  CE::Runtime::WorldSettings world = CE::Runtime::get_world_settings();
  world.timer_speed = parse_float_setting(settings, "world.timer_speed", world.timer_speed);
  world.water_threshold =
    parse_float_setting(settings, "world.water_threshold", world.water_threshold);
  world.light_pos = parse_float_array_setting<4>(settings, "world.light_pos", world.light_pos);

  world.zoom_speed = parse_float_setting(settings, "camera.zoom_speed", world.zoom_speed);
  world.panning_speed =
    parse_float_setting(settings, "camera.panning_speed", world.panning_speed);
  world.field_of_view =
    parse_float_setting(settings, "camera.field_of_view", world.field_of_view);
  world.near_clipping =
    parse_float_setting(settings, "camera.near_clipping", world.near_clipping);
  world.far_clipping =
    parse_float_setting(settings, "camera.far_clipping", world.far_clipping);
  world.camera_position =
    parse_float_array_setting<3>(settings, "camera.position", world.camera_position);
  world.arcball_tumble_mult =
    parse_float_setting(settings, "camera.arcball_tumble_mult", world.arcball_tumble_mult);
  world.arcball_pan_mult =
    parse_float_setting(settings, "camera.arcball_pan_mult", world.arcball_pan_mult);
  world.arcball_dolly_mult =
    parse_float_setting(settings, "camera.arcball_dolly_mult", world.arcball_dolly_mult);

  world.cube_shape = parse_int_setting(settings, "geometry.cube", world.cube_shape);
  world.rectangle_shape =
    parse_int_setting(settings, "geometry.rectangle", world.rectangle_shape);
  world.sphere_shape = parse_int_setting(settings, "geometry.sphere", world.sphere_shape);

  CE::Runtime::set_world_settings(world);
}

} // namespace

namespace CE::Implementation {

void ScriptChainerApp::run(const std::filesystem::path &script_path) {
  Log::text(Log::Style::header_guard);
  Log::text("{ Script Interface }");
  Log::text("Loading graph script:", script_path.string());

  const ShaderGraph graph = PythonGraphScript::load(script_path);

  Log::text("Nodes:", graph.nodes().size());
  for (const ShaderNode &node : graph.nodes()) {
    Log::text("  -", node.id, "=>", node.shader_path());
  }

  Log::text("Edges:", graph.edges().size());
  for (const GraphEdge &edge : graph.edges()) {
    Log::text("  -", edge.from, "->", edge.to);
  }

  if (graph.input().has_value()) {
    Log::text("Input:", graph.input()->node_id, graph.input()->resource);
  }
  if (graph.output().has_value()) {
    Log::text("Output:", graph.output()->node_id, graph.output()->resource);
  }

  apply_graph_runtime_settings(graph);

  const CE::Runtime::PipelineExecutionPlan plan = build_execution_plan(graph);
  CE::Runtime::set_pipeline_execution_plan(plan);

  std::unordered_map<std::string, std::string> draw_ops{};
  for (const GraphicsDrawBinding &binding : graph.graphics_draw_bindings()) {
    draw_ops[binding.pipeline_name] = binding.draw_op;
  }
  CE::Runtime::set_graphics_draw_ops(draw_ops);

  Log::text("Execution plan:");
  const auto log_group = [](const char *label, const std::vector<std::string> &pipelines) {
    for (const std::string &pipeline : pipelines) {
      Log::text("  -", label, pipeline);
    }
  };
  log_group("pre-compute:", plan.pre_graphics_compute);
  log_group("graphics:", plan.graphics);
  log_group("post-compute:", plan.post_graphics_compute);

  Log::text("Script graph loaded successfully. Runtime execution plan installed.");
  Log::text(Log::Style::header_guard);
}

} // namespace CE::Implementation
