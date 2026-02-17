#include "SceneConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace CE::Scene {

namespace {

constexpr int kDefaultRenderStage = 4;
constexpr int kMinRenderStage = 0;
constexpr int kMaxRenderStage = 5;
constexpr const char *kDefaultPreset = "default";
constexpr const char *kPresetComputeOnly = "compute_only";
constexpr const char *kPresetComputeChain = "compute_chain";

const std::vector<std::string> kDefaultComputeChain = {
    "ComputeInPlace",
    "ComputeJitter",
    "ComputeCopy",
};

const std::vector<std::string> kStage0GraphicsPipelines = {"LandscapeDebug"};
const std::vector<std::string> kStage1GraphicsPipelines = {"LandscapeStage1"};
const std::vector<std::string> kStage2GraphicsPipelines = {"LandscapeStage2"};
const std::vector<std::string> kStage3GraphicsPipelines = {"Sky", "Landscape", "TerrainBox"};
const std::vector<std::string> kStage4GraphicsPipelines = {
    "Sky", "Landscape", "TerrainBox", "Cells", "CellsFollower"};
const std::vector<std::string> kStage4PreComputePipelines = {"Engine"};

const CE::Runtime::TerrainSettings kDefaultTerrainSettings = {
  .grid_width = 100,
  .grid_height = 100,
  .alive_cells = 2000,
  .cell_size = 0.5f,
  .terrain_render_subdivisions = 2,
  .terrain_box_depth = 14.0f,
  .layer1_roughness = 0.4f,
  .layer1_octaves = 10,
  .layer1_scale = 2.2f,
  .layer1_amplitude = 16.0f,
  .layer1_exponent = 2.8f,
  .layer1_frequency = 1.6f,
  .layer1_height_offset = 0.0f,
  .layer2_roughness = 1.0f,
  .layer2_octaves = 10,
  .layer2_scale = 2.2f,
  .layer2_amplitude = 3.0f,
  .layer2_exponent = 1.5f,
  .layer2_frequency = 2.4f,
  .layer2_height_offset = 0.0f,
  .blend_factor = 0.45f,
  .absolute_height = 0.0f,
};

const CE::Runtime::WorldSettings kDefaultWorldSettings = {
  .timer_speed = 25.0f,
  .water_threshold = 0.1f,
  .water_dead_zone_margin = 2.5f,
  .water_shore_band_width = 1.0f,
  .water_border_highlight_width = 0.10f,
  .light_pos = {0.0f, 20.0f, 20.0f, 0.0f},
  .zoom_speed = 0.2f,
  .panning_speed = 0.4f,
  .field_of_view = 35.0f,
  .near_clipping = 0.25f,
  .far_clipping = 800.0f,
  .camera_position = {0.0f, 0.0f, 80.0f},
  .arcball_tumble_mult = 1.0f,
  .arcball_pan_mult = 1.4f,
  .arcball_dolly_mult = 1.3f,
  .arcball_pan_scalar = 0.65f,
  .arcball_zoom_scalar = 0.18f,
  .arcball_smoothing = 0.25f,
  .arcball_distance_pan_scale = 0.9f,
  .arcball_distance_zoom_scale = 0.8f,
  .cube_shape = 1,
  .rectangle_shape = 0,
  .sphere_shape = 2,
};

CE::Runtime::PipelineDefinition graphics_pipeline(std::initializer_list<const char *> shaders) {
  CE::Runtime::PipelineDefinition definition{};
  definition.is_compute = false;
  definition.shaders.reserve(shaders.size());
  for (const char *shader : shaders) {
    definition.shaders.emplace_back(shader);
  }
  return definition;
}

CE::Runtime::PipelineDefinition compute_pipeline(std::initializer_list<const char *> shaders) {
  CE::Runtime::PipelineDefinition definition{};
  definition.is_compute = true;
  definition.shaders.reserve(shaders.size());
  for (const char *shader : shaders) {
    definition.shaders.emplace_back(shader);
  }
  definition.work_groups = {0, 0, 0};
  return definition;
}

void add_default_pipelines(SceneConfig &spec) {
  spec.pipelines = {
      {"Cells", graphics_pipeline({"CellsVert", "CellsFrag"})},
      {"CellsFollower", graphics_pipeline({"CellsFollowerVert", "CellsFrag"})},
      {"Engine", compute_pipeline({"EngineComp"})},
      {"Landscape", graphics_pipeline({"LandscapeVert", "LandscapeFrag"})},
      {"LandscapeStatic", graphics_pipeline({"LandscapeStaticVert", "LandscapeFrag"})},
      {"LandscapeDebug", graphics_pipeline({"LandscapeVert", "LandscapeDebugFrag"})},
      {"LandscapeStage1", graphics_pipeline({"LandscapeVert", "LandscapeStage1Frag"})},
      {"LandscapeStage2", graphics_pipeline({"LandscapeVert", "LandscapeStage2Frag"})},
      {"LandscapeNormals", graphics_pipeline({"LandscapeVert", "LandscapeNormalsFrag"})},
      {"TerrainBox", graphics_pipeline({"TerrainBoxSeamVert", "TerrainBoxFrag"})},
      {"Sky", graphics_pipeline({"SkyVert", "SkyFrag"})},
      {"PostFX", compute_pipeline({"PostFXComp"})},
      {"ComputeInPlace", compute_pipeline({"ComputeInPlaceComp"})},
      {"ComputeJitter", compute_pipeline({"ComputeJitterComp"})},
      {"ComputeCopy", compute_pipeline({"ComputeCopyComp"})},
      {"SeedCells", compute_pipeline({"SeedCellsComp"})},
  };
}

const std::vector<std::string> &graphics_pipelines_for_stage(const int render_stage) {
  if (render_stage >= 4) {
    return kStage4GraphicsPipelines;
  }
  if (render_stage >= 3) {
    return kStage3GraphicsPipelines;
  }
  if (render_stage >= 2) {
    return kStage2GraphicsPipelines;
  }
  if (render_stage >= 1) {
    return kStage1GraphicsPipelines;
  }
  return kStage0GraphicsPipelines;
}

const std::vector<std::string> &pre_compute_pipelines_for_stage(const int render_stage) {
  if (render_stage >= 4) {
    return kStage4PreComputePipelines;
  }

  static const std::vector<std::string> kNone{};
  return kNone;
}

void append_pre_compute_nodes(SceneConfig &spec,
                              const std::vector<std::string> &pipelines) {
  for (const std::string &pipeline : pipelines) {
    spec.render_graph.nodes.push_back(CE::Runtime::RenderNode{
        .stage = CE::Runtime::RenderStage::PreCompute,
        .pipeline = pipeline,
        .draw_op = CE::Runtime::DrawOpId::Unknown,
    });
  }
}

void append_graphics_nodes(SceneConfig &spec,
                           const std::vector<std::string> &pipelines) {
  for (const std::string &pipeline : pipelines) {
    const auto draw_it = spec.draw_ops.find(pipeline);
    const CE::Runtime::DrawOpId draw_op =
        (draw_it != spec.draw_ops.end())
            ? CE::Runtime::draw_op_from_string(draw_it->second)
            : CE::Runtime::DrawOpId::Unknown;
    spec.render_graph.nodes.push_back(CE::Runtime::RenderNode{
        .stage = CE::Runtime::RenderStage::Graphics,
        .pipeline = pipeline,
        .draw_op = draw_op,
    });
  }
}

int parse_render_stage() {
  const char *raw = std::getenv("CE_RENDER_STAGE");
  if (!raw) {
    return kDefaultRenderStage;
  }

  char *end = nullptr;
  const long parsed = std::strtol(raw, &end, 10);
  if (end == raw || *end != '\0') {
    return kDefaultRenderStage;
  }
  if (parsed < kMinRenderStage) {
    return kDefaultRenderStage;
  }
  if (parsed > kMaxRenderStage) {
    return kMaxRenderStage;
  }
  return static_cast<int>(parsed);
}

std::string to_lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string trim(std::string_view value) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  std::size_t start = 0;
  while (start < value.size() && !not_space(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  std::size_t end = value.size();
  while (end > start && !not_space(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return std::string(value.substr(start, end - start));
}

std::vector<std::string> split_csv(const char *raw) {
  std::vector<std::string> items{};
  if (!raw || !*raw) {
    return items;
  }

  std::string_view input(raw);
  std::size_t start = 0;
  while (start <= input.size()) {
    const std::size_t comma = input.find(',', start);
    const std::size_t end = (comma == std::string_view::npos) ? input.size() : comma;
    const std::string token = trim(input.substr(start, end - start));
    if (!token.empty()) {
      items.push_back(token);
    }
    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
  }

  return items;
}

std::string workload_preset() {
  const char *raw = std::getenv("CE_WORKLOAD_PRESET");
  if (!raw) {
    return kDefaultPreset;
  }
  const std::string preset = to_lower(trim(raw));
  if (preset.empty()) {
    return kDefaultPreset;
  }
  return preset;
}

} // namespace

SceneConfig SceneConfig::defaults() {
  SceneConfig spec{};
  const int render_stage = parse_render_stage();
  const std::string preset = workload_preset();
  const std::vector<std::string> compute_chain = split_csv(std::getenv("CE_COMPUTE_CHAIN"));

  spec.terrain = kDefaultTerrainSettings;
  spec.world = kDefaultWorldSettings;
  add_default_pipelines(spec);

  spec.draw_ops = {
      {"Cells", "instanced:cells"},
      {"CellsFollower", "instanced:cells"},
      {"Landscape", "indexed:grid"},
      {"LandscapeStatic", "indexed:grid"},
      {"LandscapeDebug", "indexed:grid"},
      {"LandscapeStage1", "indexed:grid"},
      {"LandscapeStage2", "indexed:grid"},
      {"LandscapeNormals", "indexed:grid"},
      {"TerrainBox", "indexed:grid_box"},
      {"Sky", "sky_dome"},
  };

  const bool compute_only = preset == kPresetComputeOnly || preset == kPresetComputeChain;
  spec.render_graph.nodes.clear();

  if (compute_only) {
    const std::vector<std::string> &active_chain =
        compute_chain.empty() ? kDefaultComputeChain : compute_chain;

    append_pre_compute_nodes(spec, active_chain);
  } else {
    append_pre_compute_nodes(spec, pre_compute_pipelines_for_stage(render_stage));
    append_graphics_nodes(spec, graphics_pipelines_for_stage(render_stage));
  }

  return spec;
}

void SceneConfig::apply_to_runtime() const {
  CE::Runtime::set_terrain_settings(terrain);
  CE::Runtime::set_world_settings(world);
  CE::Runtime::set_pipeline_definitions(pipelines);
  CE::Runtime::set_render_graph(render_graph);
  CE::Runtime::set_graphics_draw_ops(draw_ops);
}

} // namespace CE::Scene
