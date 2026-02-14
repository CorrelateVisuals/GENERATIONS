#pragma once
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct vec2_uint_fast16_t {
  uint_fast16_t x;
  uint_fast16_t y;

  vec2_uint_fast16_t(uint_fast16_t x_val = 0, uint_fast16_t y_val = 0)
      : x(x_val), y(y_val) {}

  vec2_uint_fast16_t(const glm::ivec2 &vec)
      : x(static_cast<uint_fast16_t>(vec.x)), y(static_cast<uint_fast16_t>(vec.y)) {}
};

namespace Lib {
std::vector<float> generate_random_values(int amount, float min, float max);

double low_frequency_oscillator(double frequency);
glm::vec2 smoothstep(const glm::vec2 &xy);

std::string upper_to_lower_case(std::string string);

// Cross platform functions
std::string path(const std::string &linux_path);
std::string if_shader_compile(std::string linux_path);
} // namespace Lib
