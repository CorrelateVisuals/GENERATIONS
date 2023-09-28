#pragma once
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace Lib {
std::vector<float> generateRandomValues(int amount, float min, float max);
std::vector<uint32_t> createGridPolygons(const std::vector<uint32_t>& vertices,
                                         uint32_t gridWidth);

double lowFrequencyOscillator(double frequency);
glm::vec2 smoothstep(const glm::vec2& xy);

std::string upperToLowerCase(std::string string);

// Cross platform functions
std::string path(const std::string& linuxPath);
std::string ifShaderCompile(std::string linuxPath);
}  // namespace Lib
