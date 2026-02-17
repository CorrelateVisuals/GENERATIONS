#pragma once

// Shared utility/data-types module.
// Exists to hold common math helpers, paths, and lightweight value objects.
#include <glm/glm.hpp>

#include <string>

struct Vec2UintFast16 {
  uint_fast16_t x;
  uint_fast16_t y;

    Vec2UintFast16(uint_fast16_t x_val = 0, uint_fast16_t y_val = 0)
      : x(x_val), y(y_val) {}

    Vec2UintFast16(const glm::ivec2 &vec)
      : x(static_cast<uint_fast16_t>(vec.x)), y(static_cast<uint_fast16_t>(vec.y)) {}
};

namespace Lib {
// Cross platform functions
std::string path(const std::string &linux_path);
std::string if_shader_compile(std::string linux_path);
} // namespace Lib
