#pragma once
#include <glm/glm.hpp>

#include <vector>

class Library {
 public:
  std::vector<float> generateRandomValues(int amount, float min, float max);

  double lowFrequencyOscillator(double frequency);
  glm::vec2 smoothstep(const glm::vec2 xy);
};

inline Library lib;
