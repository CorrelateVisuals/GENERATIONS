#include "RuntimeConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>

namespace CE::Runtime {

namespace {

std::optional<PipelineExecutionPlan> active_plan{};
std::optional<RenderGraph> active_render_graph{};
std::unordered_map<std::string, PipelineDefinition> active_pipeline_definitions{};
std::unordered_map<std::string, std::string> active_graphics_draw_ops{};
std::unordered_map<std::string, DrawOpId> active_graphics_draw_op_ids{};
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

DrawOpId draw_op_from_string(std::string_view draw_op) {
  if (draw_op == "cells_instanced" || draw_op == "instanced:cells") {
    return DrawOpId::InstancedCells;
  }
  if (draw_op == "grid_indexed" || draw_op == "grid_wireframe" ||
      draw_op == "indexed:grid") {
    return DrawOpId::IndexedGrid;
  }
  if (draw_op == "indexed:grid_box") {
    return DrawOpId::IndexedGridBox;
  }
  if (draw_op == "rectangle_indexed" || draw_op == "indexed:rectangle") {
    return DrawOpId::IndexedRectangle;
  }
  if (draw_op == "indexed:cube") {
    return DrawOpId::IndexedCube;
  }
  if (draw_op == "sky_dome") {
    return DrawOpId::SkyDome;
  }
  return DrawOpId::Unknown;
}

const char *to_string(DrawOpId draw_op) {
  switch (draw_op) {
  case DrawOpId::InstancedCells:
    return "instanced:cells";
  case DrawOpId::IndexedGrid:
    return "indexed:grid";
  case DrawOpId::IndexedGridBox:
    return "indexed:grid_box";
  case DrawOpId::IndexedRectangle:
    return "indexed:rectangle";
  case DrawOpId::IndexedCube:
    return "indexed:cube";
  case DrawOpId::SkyDome:
    return "sky_dome";
  case DrawOpId::Unknown:
  default:
    return "unknown";
  }
}

void set_pipeline_execution_plan(const PipelineExecutionPlan &plan) {
  active_plan = plan;
}

const PipelineExecutionPlan *get_pipeline_execution_plan() {
  return active_plan ? &(*active_plan) : nullptr;
}

void set_render_graph(const RenderGraph &graph) {
  active_render_graph = graph;
}

const RenderGraph *get_render_graph() {
  return active_render_graph ? &(*active_render_graph) : nullptr;
}

void set_pipeline_definitions(
    const std::unordered_map<std::string, PipelineDefinition> &definitions) {
  active_pipeline_definitions = definitions;
}

const std::unordered_map<std::string, PipelineDefinition> &get_pipeline_definitions() {
  return active_pipeline_definitions;
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
  active_graphics_draw_op_ids.clear();
  for (const auto &[pipeline_name, draw_op] : draw_ops) {
    active_graphics_draw_op_ids[pipeline_name] = draw_op_from_string(draw_op);
  }
}

const std::string *get_graphics_draw_op(const std::string &pipeline_name) {
  const auto it = active_graphics_draw_ops.find(pipeline_name);
  return it == active_graphics_draw_ops.end() ? nullptr : &it->second;
}

void set_graphics_draw_op_ids(const std::unordered_map<std::string, DrawOpId> &draw_ops) {
  active_graphics_draw_op_ids = draw_ops;
  active_graphics_draw_ops.clear();
  for (const auto &[pipeline_name, draw_op] : draw_ops) {
    active_graphics_draw_ops[pipeline_name] = to_string(draw_op);
  }
}

DrawOpId get_graphics_draw_op_id(const std::string &pipeline_name) {
  const auto id_it = active_graphics_draw_op_ids.find(pipeline_name);
  if (id_it != active_graphics_draw_op_ids.end()) {
    return id_it->second;
  }

  const auto legacy_it = active_graphics_draw_ops.find(pipeline_name);
  if (legacy_it != active_graphics_draw_ops.end()) {
    return draw_op_from_string(legacy_it->second);
  }

  return DrawOpId::Unknown;
}

void clear_pipeline_execution_plan() {
  active_plan.reset();
  active_render_graph.reset();
  active_pipeline_definitions.clear();
  active_graphics_draw_ops.clear();
  active_graphics_draw_op_ids.clear();
}

} // namespace CE::Runtime
