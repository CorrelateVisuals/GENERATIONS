#pragma once
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace Library {
std::vector<float> generateRandomValues(int amount, float min, float max);

double lowFrequencyOscillator(double frequency);
glm::vec2 smoothstep(const glm::vec2 xy);

std::string path(const std::string& linuxPath);
std::string shaderPath(std::string linuxPath);
};  // namespace Library
