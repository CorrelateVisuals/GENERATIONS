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

  spec.terrain.terrain_box_depth = 14.0f;
  spec.terrain.layer1_amplitude = 16.0f;
  spec.terrain.layer1_exponent = 2.8f;
  spec.terrain.layer1_frequency = 1.6f;
  spec.terrain.layer2_amplitude = 3.0f;
  spec.terrain.layer2_exponent = 1.5f;
  spec.terrain.layer2_frequency = 2.4f;
  spec.terrain.blend_factor = 0.45f;
  spec.world.water_dead_zone_margin = 2.5f;
  spec.world.water_shore_band_width = 1.0f;
  spec.world.water_border_highlight_width = 0.10f;
  spec.world.zoom_speed = 0.2f;
  spec.world.panning_speed = 0.4f;
  spec.world.camera_position = {0.0f, 0.0f, 80.0f};
  spec.world.arcball_tumble_mult = 1.0f;
  spec.world.arcball_pan_mult = 1.4f;
  spec.world.arcball_dolly_mult = 1.3f;
  spec.world.arcball_pan_scalar = 0.65f;
  spec.world.arcball_zoom_scalar = 0.18f;
  spec.world.arcball_smoothing = 0.25f;
  spec.world.arcball_distance_pan_scale = 0.9f;
  spec.world.arcball_distance_zoom_scale = 0.8f;

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
    spec.pipelines["LandscapeStatic"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeStaticVert", "LandscapeFrag"},
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
  spec.pipelines["LandscapeNormals"] = CE::Runtime::PipelineDefinition{
      .is_compute = false,
      .shaders = {"LandscapeVert", "LandscapeNormalsFrag"},
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
  spec.pipelines["ComputeInPlace"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"ComputeInPlaceComp"},
      .work_groups = {0, 0, 0},
  };
  spec.pipelines["ComputeJitter"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"ComputeJitterComp"},
      .work_groups = {0, 0, 0},
  };
  spec.pipelines["ComputeCopy"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"ComputeCopyComp"},
      .work_groups = {0, 0, 0},
  };
  spec.pipelines["SeedCells"] = CE::Runtime::PipelineDefinition{
      .is_compute = true,
      .shaders = {"SeedCellsComp"},
      .work_groups = {0, 0, 0},
  };

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
  if (compute_only) {
    const std::vector<std::string> &active_chain =
        compute_chain.empty() ? kDefaultComputeChain : compute_chain;

    spec.render_graph.nodes.clear();
    for (const std::string &pipeline : active_chain) {
      spec.render_graph.nodes.push_back(CE::Runtime::RenderNode{
          .stage = CE::Runtime::RenderStage::PreCompute,
          .pipeline = pipeline,
          .draw_op = CE::Runtime::DrawOpId::Unknown,
      });
    }
  } else {
    std::vector<std::string> graphics_pipelines = kStage0GraphicsPipelines;
    std::vector<std::string> pre_compute_pipelines{};

    if (render_stage >= 1) {
      graphics_pipelines = kStage1GraphicsPipelines;
    }
    if (render_stage >= 2) {
      graphics_pipelines = kStage2GraphicsPipelines;
    }
    if (render_stage >= 3) {
      graphics_pipelines = kStage3GraphicsPipelines;
    }
    if (render_stage >= 4) {
      pre_compute_pipelines = kStage4PreComputePipelines;
      graphics_pipelines = kStage4GraphicsPipelines;
    }

    spec.render_graph.nodes.clear();
    for (const std::string &pipeline : pre_compute_pipelines) {
      spec.render_graph.nodes.push_back(CE::Runtime::RenderNode{
          .stage = CE::Runtime::RenderStage::PreCompute,
          .pipeline = pipeline,
          .draw_op = CE::Runtime::DrawOpId::Unknown,
      });
    }

    for (const std::string &pipeline : graphics_pipelines) {
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
