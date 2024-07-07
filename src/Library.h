#pragma once
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct uivec2_fast16_t {
  uint_fast16_t x;
  uint_fast16_t y;
};

struct ivec4 {
  int r, g, b, a;
};

struct vec4 {
  float r, g, b, a;
};

namespace Lib {
std::vector<float> generateRandomValues(int amount, float min, float max);

double lowFrequencyOscillator(double frequency);
glm::vec2 smoothstep(const glm::vec2& xy);

std::string upperToLowerCase(std::string string);

// Cross platform functions
std::string path(const std::string& linuxPath);
std::string ifShaderCompile(std::string linuxPath);
}  // namespace Lib
