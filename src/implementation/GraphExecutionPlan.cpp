#include "GraphExecutionPlan.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace {

using StepName = std::string;
using StepEdge = std::pair<StepName, StepName>;

struct StepTraits {
  bool has_compute{false};
  bool has_graphics{false};
};

std::vector<StepName> topo_sort_steps(const std::vector<StepName> &steps,
                                      const std::vector<StepEdge> &edges,
                                      const std::vector<StepName> &preferred_order) {
  std::unordered_set<StepName> step_set(steps.begin(), steps.end());
  std::unordered_map<StepName, std::vector<StepName>> adjacency{};
  std::unordered_map<StepName, std::unordered_set<StepName>> edge_seen{};
  std::unordered_map<StepName, uint32_t> indegree{};
  std::unordered_set<StepName> emitted{};

  for (const StepName &step : steps) {
    indegree.emplace(step, 0);
  }

  for (const StepEdge &edge : edges) {
    const StepName &from = edge.first;
    const StepName &to = edge.second;
    if (!step_set.contains(from) || !step_set.contains(to) || from == to) {
      continue;
    }
    if (edge_seen[from].insert(to).second) {
      adjacency[from].push_back(to);
      indegree[to]++;
    }
  }

  std::vector<StepName> ordered{};
  bool progress = true;
  while (ordered.size() < steps.size() && progress) {
    progress = false;
    for (const StepName &step : preferred_order) {
      if (!step_set.contains(step) || emitted.contains(step)) {
        continue;
      }
      if (indegree[step] == 0) {
        ordered.push_back(step);
        emitted.insert(step);
        progress = true;

        if (!adjacency.contains(step)) {
          continue;
        }
        for (const StepName &next : adjacency[step]) {
          --indegree[next];
        }
      }
    }
  }

  if (ordered.size() != steps.size()) {
    ordered.clear();
    for (const StepName &step : preferred_order) {
      if (step_set.contains(step)) {
        ordered.push_back(step);
      }
    }
  }

  return ordered;
}

} // namespace

namespace CE::Implementation {

CE::Runtime::PipelineExecutionPlan build_execution_plan(const ShaderGraph &graph) {
  std::unordered_map<std::string, StepName> step_for_node{};
  std::unordered_map<StepName, StepTraits> step_traits{};
  std::unordered_set<StepName> steps{};
  std::vector<StepName> discovered_steps{};

  for (const ShaderNode &node : graph.nodes()) {
    const StepName &step = node.shader_name;
    if (step.empty()) {
      continue;
    }
    step_for_node[node.id] = step;

    if (!steps.contains(step)) {
      discovered_steps.push_back(step);
    }
    steps.insert(step);

    StepTraits &traits = step_traits[step];
    if (node.stage == ShaderStage::Comp) {
      traits.has_compute = true;
    } else {
      traits.has_graphics = true;
    }
  }

  std::vector<StepEdge> step_edges{};
  for (const GraphEdge &edge : graph.edges()) {
    if (!step_for_node.contains(edge.from) || !step_for_node.contains(edge.to)) {
      continue;
    }
    step_edges.emplace_back(step_for_node.at(edge.from), step_for_node.at(edge.to));
  }

  const std::vector<StepName> step_list(steps.begin(), steps.end());
  const std::vector<StepName> ordered_steps = topo_sort_steps(step_list, step_edges, discovered_steps);

  std::size_t first_graphics_index = ordered_steps.size();
  for (std::size_t i = 0; i < ordered_steps.size(); i++) {
    const StepName &step = ordered_steps[i];
    if (step_traits[step].has_graphics) {
      first_graphics_index = i;
      break;
    }
  }

  CE::Runtime::PipelineExecutionPlan plan{};
  for (std::size_t i = 0; i < ordered_steps.size(); i++) {
    const StepName &step = ordered_steps[i];
    const StepTraits &traits = step_traits[step];

    if (traits.has_graphics) {
      plan.graphics.push_back(step);
      continue;
    }

    if (i < first_graphics_index) {
      plan.pre_graphics_compute.push_back(step);
    } else {
      plan.post_graphics_compute.push_back(step);
    }
  }

  return plan;
}

} // namespace CE::Implementation
