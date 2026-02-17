#pragma once

// Declarative scene preset that populates runtime configuration.
// Exists to define default terrain/world/pipeline graph in one place.

#include "world/RuntimeConfig.h"

namespace CE::Scene {

struct SceneConfig {
  CE::Runtime::TerrainSettings terrain{};
  CE::Runtime::WorldSettings world{};
  std::unordered_map<std::string, CE::Runtime::PipelineDefinition> pipelines{};
  CE::Runtime::RenderGraph render_graph{};
  std::unordered_map<std::string, std::string> draw_ops{};

  static SceneConfig defaults();
  void apply_to_runtime() const;
};

} // namespace CE::Scene
