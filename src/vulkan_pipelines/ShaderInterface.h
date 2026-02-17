#pragma once

#include <glm/glm.hpp>

namespace CE::ShaderInterface {

struct alignas(16) ParameterUBO {
  glm::vec4 light{};
  glm::ivec2 grid_xy{};
  float water_threshold{0.0f};
  float cell_size{0.0f};
  glm::vec4 water_rules{2.4f, 1.2f, 0.08f, 0.0f};
  glm::mat4 model{};
  glm::mat4 view{};
  glm::mat4 projection{};
};

} // namespace CE::ShaderInterface
