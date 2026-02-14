#pragma once

#include <unordered_map>
#include <string>
#include <vector>

namespace CE::Runtime {

// Parses an environment-style boolean string.
//
// Returns true only for explicit truthy values:
//   "1", "true", "on" (case-insensitive)
// Any other value (including nullptr) returns false.
bool env_truthy(const char *value);

// Reads an environment variable by name and parses it via env_truthy().
//
// This centralizes env-flag semantics across the app so different call sites
// do not drift over time.
bool env_flag_enabled(const char *name);

struct PipelineExecutionPlan {
  std::vector<std::string> pre_graphics_compute{};
  std::vector<std::string> graphics{};
  std::vector<std::string> post_graphics_compute{};
};

void set_pipeline_execution_plan(const PipelineExecutionPlan &plan);
const PipelineExecutionPlan *get_pipeline_execution_plan();

void set_graphics_draw_ops(const std::unordered_map<std::string, std::string> &draw_ops);
const std::string *get_graphics_draw_op(const std::string &pipeline_name);

void clear_pipeline_execution_plan();

} // namespace CE::Runtime
