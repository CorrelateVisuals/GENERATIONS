#pragma once

#include "core/RuntimeConfig.h"

#include <unordered_map>

namespace CE::Implementation {

struct ImplementationSpec {
  CE::Runtime::TerrainSettings terrain{};
  CE::Runtime::WorldSettings world{};
  std::unordered_map<std::string, CE::Runtime::PipelineDefinition> pipelines{};
  CE::Runtime::RenderGraph render_graph{};
  std::unordered_map<std::string, std::string> draw_ops{};
};

ImplementationSpec default_spec();

} // namespace CE::Implementation
