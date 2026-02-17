#pragma once

// CPU-side mirror of shader parameter layout.
// Exists to guarantee host/shader alignment for shared UBO contract.

#include <glm/glm.hpp>

namespace CE::ShaderInterface {

// Generated from src/resources/ParameterUBO.schema. Do not edit by hand.
struct ParameterUBO {
  glm::vec4 light{};
  glm::ivec2 grid_xy{};
  float water_threshold{};
  float cell_size{};
  glm::vec4 water_rules{2.4f, 1.2f, 0.08f, 0.0f};
  alignas(16) glm::mat4 model{};
  alignas(16) glm::mat4 view{};
  alignas(16) glm::mat4 projection{};

  ParameterUBO(glm::vec4 l, glm::ivec2 xy, float w, float s, glm::vec4 rules)
      : light(l), grid_xy(xy), water_threshold(w), cell_size(s), water_rules(rules) {}
};

} // namespace CE::ShaderInterface
