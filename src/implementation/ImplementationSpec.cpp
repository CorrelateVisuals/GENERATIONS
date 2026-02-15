#include "ImplementationSpec.h"

#include <algorithm>
#include <cctype>
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
    return "default";
  }
  const std::string preset = to_lower(trim(raw));
  if (preset.empty()) {
    return "default";
  }
  return preset;
}
}

ImplementationSpec default_spec() {
  ImplementationSpec spec{};
  const int render_stage = parse_render_stage();
  const std::string preset = workload_preset();
  const std::vector<std::string> compute_chain = split_csv(std::getenv("CE_COMPUTE_CHAIN"));

  spec.terrain.grid_width = 100;
  spec.terrain.grid_height = 100;
  spec.terrain.alive_cells = 4000;
  spec.terrain.cell_size = 0.5f;
  spec.terrain.terrain_render_subdivisions = 2;
  spec.terrain.terrain_box_depth = 14.0f;
  spec.terrain.layer1_roughness = 0.4f;
  spec.terrain.layer1_octaves = 10;
  spec.terrain.layer1_scale = 2.2f;
  spec.terrain.layer1_amplitude = 16.0f;
  spec.terrain.layer1_exponent = 2.8f;
  spec.terrain.layer1_frequency = 1.6f;
  spec.terrain.layer1_height_offset = 0.0f;
  spec.terrain.layer2_roughness = 1.0f;
  spec.terrain.layer2_octaves = 10;
  spec.terrain.layer2_scale = 2.2f;
  spec.terrain.layer2_amplitude = 3.0f;
  spec.terrain.layer2_exponent = 1.5f;
  spec.terrain.layer2_frequency = 2.4f;
  spec.terrain.layer2_height_offset = 0.0f;
  spec.terrain.blend_factor = 0.45f;
  spec.terrain.absolute_height = 0.0f;

  spec.world.timer_speed = 25.0f;
  spec.world.water_threshold = 0.1f;
  spec.world.water_dead_zone_margin = 2.5f;
  spec.world.water_shore_band_width = 1.0f;
  spec.world.water_border_highlight_width = 0.10f;
  spec.world.light_pos = {0.0f, 20.0f, 20.0f, 0.0f};
  spec.world.zoom_speed = 0.2f;
  spec.world.panning_speed = 0.4f;
  spec.world.field_of_view = 35.0f;
  spec.world.near_clipping = 0.25f;
  spec.world.far_clipping = 800.0f;
  spec.world.camera_position = {0.0f, 0.0f, 80.0f};
  spec.world.arcball_tumble_mult = 1.0f;
  spec.world.arcball_pan_mult = 1.4f;
  spec.world.arcball_dolly_mult = 1.3f;
  spec.world.arcball_pan_scalar = 0.65f;
  spec.world.arcball_zoom_scalar = 0.18f;
  spec.world.arcball_smoothing = 0.25f;
  spec.world.arcball_distance_pan_scale = 0.9f;
  spec.world.arcball_distance_zoom_scale = 0.8f;
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

    const bool compute_only = preset == "compute_only" || preset == "compute_chain";
    if (compute_only) {
      const std::vector<std::string> default_chain = {
          "ComputeInPlace",
          "ComputeJitter",
          "ComputeCopy",
      };
      const std::vector<std::string> &active_chain =
          compute_chain.empty() ? default_chain : compute_chain;

      spec.render_graph.nodes.clear();
      for (const std::string &pipeline : active_chain) {
        spec.render_graph.nodes.push_back(CE::Runtime::RenderNode{
            .stage = CE::Runtime::RenderStage::PreCompute,
            .pipeline = pipeline,
            .draw_op = CE::Runtime::DrawOpId::Unknown,
        });
      }
    } else {
      std::vector<std::string> graphics_pipelines = {"LandscapeDebug"};
      std::vector<std::string> pre_compute_pipelines{};

      if (render_stage >= 1) {
        graphics_pipelines = {"LandscapeStage1"};
      }
      if (render_stage >= 2) {
        graphics_pipelines = {"LandscapeStage2"};
      }
      if (render_stage >= 3) {
        graphics_pipelines = {"Sky", "Landscape", "TerrainBox"};
      }
      if (render_stage >= 4) {
        pre_compute_pipelines = {"Engine"};
        graphics_pipelines = {"Sky", "Landscape", "TerrainBox", "CellsFollower", "Cells"};
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

  spec.draw_ops = {
      {"Cells", "instanced:cells"},
      {"CellsFollower", "instanced:cells"},
      {"Landscape", "indexed:grid"},
      {"LandscapeDebug", "indexed:grid"},
      {"LandscapeStage1", "indexed:grid"},
      {"LandscapeStage2", "indexed:grid"},
      {"LandscapeNormals", "indexed:grid"},
        {"TerrainBox", "indexed:grid_box"},
      {"Sky", "sky_dome"},
  };

  return spec;
}

} // namespace CE::Implementation
