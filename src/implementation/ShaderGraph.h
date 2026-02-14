#pragma once

#include <optional>
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace CE::Implementation {

enum class ShaderStage {
  Vert,
  Frag,
  Comp,
  Tesc,
  Tese,
  Geom,
  Unknown,
};

struct ShaderNode {
  std::string id;
  std::string shader_name;
  ShaderStage stage{ShaderStage::Unknown};

  std::string shader_path() const;
};

struct GraphEdge {
  std::string from;
  std::string to;
};

struct GraphEndpoint {
  std::string node_id;
  std::string resource;
};

struct GraphicsDrawBinding {
  std::string pipeline_name;
  std::string draw_op;
};

struct PipelineDefinition {
  std::string pipeline_name;
  bool is_compute{false};
  std::vector<std::string> shader_ids{};
  std::array<uint32_t, 3> work_groups{0, 0, 0};
};

class ShaderGraph {
public:
  bool add_node(const ShaderNode &node, std::string &error);
  bool add_edge(const GraphEdge &edge, std::string &error);
  bool add_graphics_draw_binding(const GraphicsDrawBinding &binding, std::string &error);
  bool add_pipeline_definition(const PipelineDefinition &definition, std::string &error);
  bool add_setting(const std::string &key, const std::string &value, std::string &error);

  void set_input(const GraphEndpoint &input_endpoint);
  void set_output(const GraphEndpoint &output_endpoint);

  bool validate(std::string &error) const;

  const std::vector<ShaderNode> &nodes() const { return nodes_; }
  const std::vector<GraphEdge> &edges() const { return edges_; }
  const std::vector<GraphicsDrawBinding> &graphics_draw_bindings() const {
    return graphics_draw_bindings_;
  }
  const std::vector<PipelineDefinition> &pipeline_definitions() const {
    return pipeline_definitions_;
  }
  const std::optional<GraphEndpoint> &input() const { return input_; }
  const std::optional<GraphEndpoint> &output() const { return output_; }
  const std::unordered_map<std::string, std::string> &settings() const {
    return settings_;
  }

private:
  std::vector<ShaderNode> nodes_;
  std::vector<GraphEdge> edges_;
  std::vector<GraphicsDrawBinding> graphics_draw_bindings_;
  std::vector<PipelineDefinition> pipeline_definitions_;
  std::unordered_map<std::string, std::size_t> node_index_by_id_;
  std::unordered_map<std::string, std::size_t> graphics_draw_index_by_pipeline_name_;
  std::unordered_map<std::string, std::size_t> pipeline_definition_index_by_name_;
  std::unordered_map<std::string, std::string> settings_;
  std::optional<GraphEndpoint> input_;
  std::optional<GraphEndpoint> output_;
};

ShaderStage shader_stage_from_extension(const std::string &extension);
std::string shader_stage_to_extension(ShaderStage stage);

} // namespace CE::Implementation
