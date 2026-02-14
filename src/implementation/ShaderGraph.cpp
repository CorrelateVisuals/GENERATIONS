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
