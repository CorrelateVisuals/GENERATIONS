#include "ScriptChainerApp.h"

#include "GraphExecutionPlan.h"
#include "PythonGraphScript.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
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

int shader_stage_rank(const CE::Implementation::ShaderStage stage) {
  switch (stage) {
  case CE::Implementation::ShaderStage::Vert:
    return 0;
  case CE::Implementation::ShaderStage::Tesc:
    return 1;
  case CE::Implementation::ShaderStage::Tese:
    return 2;
  case CE::Implementation::ShaderStage::Geom:
    return 3;
  case CE::Implementation::ShaderStage::Frag:
    return 4;
  case CE::Implementation::ShaderStage::Comp:
    return 5;
  case CE::Implementation::ShaderStage::Unknown:
    return 6;
  }
  return 6;
}

std::array<uint32_t, 3> parse_workgroups_setting(
    const std::unordered_map<std::string, std::string> &settings,
    const std::string &pipeline_name,
    const std::array<uint32_t, 3> fallback) {
  const std::string key = "workgroups." + pipeline_name;
  const auto it = settings.find(key);
  if (it == settings.end()) {
    return fallback;
  }

  std::array<float, 3> parsed =
      parse_float_array_setting<3>(settings, key.c_str(),
                                   {static_cast<float>(fallback[0]),
                                    static_cast<float>(fallback[1]),
                                    static_cast<float>(fallback[2])});

  return {static_cast<uint32_t>(std::max(1.0f, parsed[0])),
          static_cast<uint32_t>(std::max(1.0f, parsed[1])),
          static_cast<uint32_t>(std::max(1.0f, parsed[2]))};
}

void install_pipeline_definitions_from_graph(const CE::Implementation::ShaderGraph &graph) {
  if (!graph.pipeline_definitions().empty()) {
    std::unordered_map<std::string, CE::Runtime::PipelineDefinition> definitions{};
    const CE::Runtime::TerrainSettings &terrain = CE::Runtime::get_terrain_settings();

    for (const CE::Implementation::PipelineDefinition &pipeline :
         graph.pipeline_definitions()) {
      CE::Runtime::PipelineDefinition definition{};
      definition.is_compute = pipeline.is_compute;
      definition.shaders = pipeline.shader_ids;

      if (pipeline.is_compute) {
        std::array<uint32_t, 3> fallback{1, 1, 1};
        if (pipeline.pipeline_name == "Engine") {
          fallback = {static_cast<uint32_t>(terrain.grid_width + 31) / 32,
                      static_cast<uint32_t>(terrain.grid_height + 31) / 32,
                      1};
        }
        definition.work_groups = pipeline.work_groups;
        if (definition.work_groups[0] == 0 || definition.work_groups[1] == 0 ||
            definition.work_groups[2] == 0) {
          definition.work_groups = fallback;
        }
      }

      definitions[pipeline.pipeline_name] = definition;
    }

    CE::Runtime::set_pipeline_definitions(definitions);
    return;
  }

  std::unordered_map<std::string, CE::Runtime::PipelineDefinition> definitions{};
  std::unordered_map<std::string, CE::Implementation::ShaderStage> node_stage_by_id{};
  std::unordered_map<std::string,
                     std::unordered_map<CE::Implementation::ShaderStage, std::string>>
      stage_node_by_pipeline{};

  const auto &settings = graph.settings();
  const CE::Runtime::TerrainSettings &terrain = CE::Runtime::get_terrain_settings();

  for (const CE::Implementation::ShaderNode &node : graph.nodes()) {
    CE::Runtime::PipelineDefinition &definition = definitions[node.shader_name];
    definition.is_compute = definition.is_compute ||
                            (node.stage == CE::Implementation::ShaderStage::Comp);
    definition.shaders.push_back(node.id);
    node_stage_by_id[node.id] = node.stage;
    stage_node_by_pipeline[node.shader_name].try_emplace(node.stage, node.id);
  }

  for (auto &[pipeline_name, definition] : definitions) {
    if (!definition.is_compute) {
      bool has_vert = false;
      bool has_frag = false;
      for (const std::string &shader_id : definition.shaders) {
        const auto stage_it = node_stage_by_id.find(shader_id);
        if (stage_it == node_stage_by_id.end()) {
          continue;
        }
        has_vert = has_vert || (stage_it->second == CE::Implementation::ShaderStage::Vert);
        has_frag = has_frag || (stage_it->second == CE::Implementation::ShaderStage::Frag);
      }

      constexpr std::string_view wireframe_suffix = "WireFrame";
      if ((!has_vert || !has_frag) && pipeline_name.ends_with(wireframe_suffix)) {
        const std::string base_pipeline_name =
            pipeline_name.substr(0, pipeline_name.size() - wireframe_suffix.size());
        const auto base_pipeline_it = stage_node_by_pipeline.find(base_pipeline_name);
        if (base_pipeline_it == stage_node_by_pipeline.end()) {
          continue;
        }

        if (!has_vert) {
          const auto base_vert_it =
              base_pipeline_it->second.find(CE::Implementation::ShaderStage::Vert);
          if (base_vert_it != base_pipeline_it->second.end()) {
            definition.shaders.push_back(base_vert_it->second);
          }
        }
        if (!has_frag) {
          const auto base_frag_it =
              base_pipeline_it->second.find(CE::Implementation::ShaderStage::Frag);
          if (base_frag_it != base_pipeline_it->second.end()) {
            definition.shaders.push_back(base_frag_it->second);
          }
        }
      }
    }

    std::sort(definition.shaders.begin(),
              definition.shaders.end(),
              [&node_stage_by_id](const std::string &a, const std::string &b) {
                const auto stage_a_it = node_stage_by_id.find(a);
                const auto stage_b_it = node_stage_by_id.find(b);
                const CE::Implementation::ShaderStage stage_a =
                    (stage_a_it == node_stage_by_id.end())
                        ? CE::Implementation::ShaderStage::Unknown
                        : stage_a_it->second;
                const CE::Implementation::ShaderStage stage_b =
                    (stage_b_it == node_stage_by_id.end())
                        ? CE::Implementation::ShaderStage::Unknown
                        : stage_b_it->second;
                const int rank_a = shader_stage_rank(stage_a);
                const int rank_b = shader_stage_rank(stage_b);
                if (rank_a != rank_b) {
                  return rank_a < rank_b;
                }
                return a < b;
              });

    definition.shaders.erase(
        std::unique(definition.shaders.begin(), definition.shaders.end()),
        definition.shaders.end());

    if (definition.is_compute) {
      std::array<uint32_t, 3> fallback{1, 1, 1};
      if (pipeline_name == "Engine") {
        fallback = {static_cast<uint32_t>(terrain.grid_width + 31) / 32,
                    static_cast<uint32_t>(terrain.grid_height + 31) / 32,
                    1};
      }
      definition.work_groups = parse_workgroups_setting(settings, pipeline_name, fallback);
    }
  }

  CE::Runtime::set_pipeline_definitions(definitions);
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
  install_pipeline_definitions_from_graph(graph);

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
