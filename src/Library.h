#pragma once
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace Lib {
std::vector<float> generateRandomValues(int amount, float min, float max);

double lowFrequencyOscillator(double frequency);
glm::vec2 smoothstep(const glm::vec2& xy);

// Cross platform functions
std::string path(const std::string& linuxPath);
std::string ifShaderAdaptation(std::string& linuxPath);
};  // namespace Lib
