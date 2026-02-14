#include "ScriptChainerApp.h"

#include "PythonGraphScript.h"
#include "core/Log.h"

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

  Log::text("Script graph loaded successfully. Execution bridge to Vulkan pipelines is next.");
  Log::text(Log::Style::header_guard);
}

} // namespace CE::Implementation
