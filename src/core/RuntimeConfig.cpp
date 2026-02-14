#include "RuntimeConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>

namespace CE::Runtime {

namespace {

std::optional<PipelineExecutionPlan> active_plan{};
std::unordered_map<std::string, std::string> active_graphics_draw_ops{};
TerrainSettings active_terrain_settings{};
WorldSettings active_world_settings{};

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

void set_terrain_settings(const TerrainSettings &settings) {
  active_terrain_settings = settings;
}

const TerrainSettings &get_terrain_settings() {
  return active_terrain_settings;
}

void set_world_settings(const WorldSettings &settings) {
  active_world_settings = settings;
}

const WorldSettings &get_world_settings() {
  return active_world_settings;
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
