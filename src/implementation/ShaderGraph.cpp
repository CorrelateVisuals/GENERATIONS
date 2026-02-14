#include "ShaderGraph.h"

#include <sstream>

namespace {

bool is_blank(const std::string &value) {
  return value.empty();
}

} // namespace

namespace CE::Implementation {

std::string ShaderNode::shader_path() const {
  return "shaders/" + shader_name + "." + shader_stage_to_extension(stage);
}

bool ShaderGraph::add_node(const ShaderNode &node, std::string &error) {
  if (is_blank(node.id)) {
    error = "node id cannot be blank";
    return false;
  }
  if (is_blank(node.shader_name)) {
    error = "shader name cannot be blank for node " + node.id;
    return false;
  }
  if (node.stage == ShaderStage::Unknown) {
    error = "shader stage is unknown for node " + node.id;
    return false;
  }
  if (node_index_by_id_.contains(node.id)) {
    error = "duplicate node id: " + node.id;
    return false;
  }

  node_index_by_id_[node.id] = nodes_.size();
  nodes_.push_back(node);
  return true;
}

bool ShaderGraph::add_edge(const GraphEdge &edge, std::string &error) {
  if (!node_index_by_id_.contains(edge.from)) {
    error = "edge source node does not exist: " + edge.from;
    return false;
  }
  if (!node_index_by_id_.contains(edge.to)) {
    error = "edge target node does not exist: " + edge.to;
    return false;
  }
  edges_.push_back(edge);
  return true;
}

bool ShaderGraph::add_graphics_draw_binding(const GraphicsDrawBinding &binding,
                                            std::string &error) {
  if (is_blank(binding.pipeline_name)) {
    error = "graphics draw binding pipeline_name cannot be blank";
    return false;
  }
  if (is_blank(binding.draw_op)) {
    error = "graphics draw binding draw_op cannot be blank for pipeline " +
            binding.pipeline_name;
    return false;
  }
  if (graphics_draw_index_by_pipeline_name_.contains(binding.pipeline_name)) {
    error = "duplicate graphics draw binding pipeline_name: " + binding.pipeline_name;
    return false;
  }

  graphics_draw_index_by_pipeline_name_[binding.pipeline_name] =
      graphics_draw_bindings_.size();
  graphics_draw_bindings_.push_back(binding);
  return true;
}

bool ShaderGraph::add_pipeline_definition(const PipelineDefinition &definition,
                                          std::string &error) {
  if (is_blank(definition.pipeline_name)) {
    error = "pipeline definition name cannot be blank";
    return false;
  }
  if (pipeline_definition_index_by_name_.contains(definition.pipeline_name)) {
    error = "duplicate pipeline definition: " + definition.pipeline_name;
    return false;
  }
  if (definition.shader_ids.empty()) {
    error = "pipeline definition has no shaders: " + definition.pipeline_name;
    return false;
  }

  pipeline_definition_index_by_name_[definition.pipeline_name] =
      pipeline_definitions_.size();
  pipeline_definitions_.push_back(definition);
  return true;
}

bool ShaderGraph::add_setting(const std::string &key,
                              const std::string &value,
                              std::string &error) {
  if (is_blank(key)) {
    error = "setting key cannot be blank";
    return false;
  }
  if (is_blank(value)) {
    error = "setting value cannot be blank for key " + key;
    return false;
  }
  if (settings_.contains(key)) {
    error = "duplicate setting key: " + key;
    return false;
  }

  settings_[key] = value;
  return true;
}

void ShaderGraph::set_input(const GraphEndpoint &input_endpoint) {
  input_ = input_endpoint;
}

void ShaderGraph::set_output(const GraphEndpoint &output_endpoint) {
  output_ = output_endpoint;
}

bool ShaderGraph::validate(std::string &error) const {
  if (nodes_.empty()) {
    error = "graph has no nodes";
    return false;
  }
  if (!input_.has_value()) {
    error = "graph is missing input endpoint";
    return false;
  }
  if (!output_.has_value()) {
    error = "graph is missing output endpoint";
    return false;
  }
  if (!node_index_by_id_.contains(input_->node_id)) {
    error = "input endpoint node does not exist: " + input_->node_id;
    return false;
  }
  if (!node_index_by_id_.contains(output_->node_id)) {
    error = "output endpoint node does not exist: " + output_->node_id;
    return false;
  }

  for (const PipelineDefinition &pipeline : pipeline_definitions_) {
    bool has_compute = false;
    bool has_graphics = false;

    for (const std::string &shader_id : pipeline.shader_ids) {
      if (!node_index_by_id_.contains(shader_id)) {
        error = "pipeline definition references unknown shader node: " + shader_id;
        return false;
      }

      const ShaderNode &node = nodes_[node_index_by_id_.at(shader_id)];
      if (node.stage == ShaderStage::Comp) {
        has_compute = true;
      } else {
        has_graphics = true;
      }
    }

    if (pipeline.is_compute && !has_compute) {
      error = "compute pipeline definition has no compute shader: " +
              pipeline.pipeline_name;
      return false;
    }

    if (!pipeline.is_compute && !has_graphics) {
      error = "graphics pipeline definition has no graphics shaders: " +
              pipeline.pipeline_name;
      return false;
    }
  }

  for (const GraphicsDrawBinding &binding : graphics_draw_bindings_) {
    bool pipeline_exists = pipeline_definition_index_by_name_.contains(binding.pipeline_name);
    if (!pipeline_exists) {
      for (const ShaderNode &node : nodes_) {
        if (node.shader_name == binding.pipeline_name && node.stage != ShaderStage::Comp) {
          pipeline_exists = true;
          break;
        }
      }
    }

    if (!pipeline_exists) {
      error = "graphics draw binding references unknown graphics pipeline: " +
              binding.pipeline_name;
      return false;
    }
  }

  return true;
}

ShaderStage shader_stage_from_extension(const std::string &extension) {
  if (extension == "vert") {
    return ShaderStage::Vert;
  }
  if (extension == "frag") {
    return ShaderStage::Frag;
  }
  if (extension == "comp") {
    return ShaderStage::Comp;
  }
  if (extension == "tesc") {
    return ShaderStage::Tesc;
  }
  if (extension == "tese") {
    return ShaderStage::Tese;
  }
  if (extension == "geom") {
    return ShaderStage::Geom;
  }
  return ShaderStage::Unknown;
}

std::string shader_stage_to_extension(ShaderStage stage) {
  switch (stage) {
  case ShaderStage::Vert:
    return "vert";
  case ShaderStage::Frag:
    return "frag";
  case ShaderStage::Comp:
    return "comp";
  case ShaderStage::Tesc:
    return "tesc";
  case ShaderStage::Tese:
    return "tese";
  case ShaderStage::Geom:
    return "geom";
  case ShaderStage::Unknown:
    return "unknown";
  }
  return "unknown";
}

} // namespace CE::Implementation
