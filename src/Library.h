#pragma once
#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace Library {
  std::vector<float> generateRandomValues(int amount, float min, float max);

  double lowFrequencyOscillator(double frequency);
  glm::vec2 smoothstep(const glm::vec2 xy);

  std::string linuxToWindowsPath(const std::string& linuxImagePath);

};

