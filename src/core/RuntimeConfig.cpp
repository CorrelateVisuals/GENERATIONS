#include "RuntimeConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>

namespace CE::Runtime {

namespace {

std::optional<PipelineExecutionPlan> active_plan{};
std::unordered_map<std::string, std::string> active_graphics_draw_ops{};

} // namespace

bool env_truthy(const char *value) {
  if (!value) {
    return false;
  }

  std::string mode(value);
  std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  return mode == "1" || mode == "true" || mode == "on";
}

bool env_flag_enabled(const char *name) {
  if (!name) {
    return false;
  }
  return env_truthy(std::getenv(name));
}

void set_pipeline_execution_plan(const PipelineExecutionPlan &plan) {
  active_plan = plan;
}

const PipelineExecutionPlan *get_pipeline_execution_plan() {
  return active_plan ? &(*active_plan) : nullptr;
}

void set_graphics_draw_ops(const std::unordered_map<std::string, std::string> &draw_ops) {
  active_graphics_draw_ops = draw_ops;
}

const std::string *get_graphics_draw_op(const std::string &pipeline_name) {
  const auto it = active_graphics_draw_ops.find(pipeline_name);
  return it == active_graphics_draw_ops.end() ? nullptr : &it->second;
}

void clear_pipeline_execution_plan() {
  active_plan.reset();
  active_graphics_draw_ops.clear();
}

} // namespace CE::Runtime
