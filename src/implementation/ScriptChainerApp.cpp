#include "ScriptChainerApp.h"

#include "GraphExecutionPlan.h"
#include "PythonGraphScript.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"

#include <unordered_map>

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
