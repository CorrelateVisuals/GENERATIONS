#include <glm/glm.hpp>

#include "Library.h"

#include <chrono>
#include <numbers>
#include <random>

std::vector<float> Library::generateRandomValues(int amount,
                                                 float min,
                                                 float max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(min, max);
  std::vector<float> randomValues(amount);
  for (size_t i = 0; i < amount; ++i) {
    randomValues[i] = dis(gen);
  }
  return randomValues;
}

double Library::lowFrequencyOscillator(double frequency) {
  using namespace std::chrono;
  static const auto start_time = high_resolution_clock::now();
  const auto time_elapsed =
      duration_cast<milliseconds>(high_resolution_clock::now() - start_time)
          .count();
  const double period = 1000.0 / frequency;
  const double angle = time_elapsed * frequency * 2 * std::numbers::pi / 1000.0;
  return 0.5 * (1 + std::sin(angle));
}

glm::vec2 Library::smoothstep(const glm::vec2 xy) {
  constexpr float startInput = 0.0f;
  constexpr float endInput = 1.0f;
  constexpr float minIncrease = -0.1f;
  constexpr float maxIncrease = 0.1f;

  float tX = (xy.x - startInput) / (endInput - startInput);
  float tY = (xy.y - startInput) / (endInput - startInput);

  float smoothX = tX * tX * (3.0f - 2.0f * tX);
  float smoothY = tY * tY * (3.0f - 2.0f * tY);

  glm::vec2 increase = glm::mix(glm::vec2(minIncrease), glm::vec2(maxIncrease),
                                glm::vec2(smoothX, smoothY));
  return increase;
}

std::string Library::linuxToWindowsPath(const std::string& linuxImagePath) {
  std::string windowsImagePathResult = "..\\" + linuxImagePath;
  // Replace forward slashes with backslashes
  for (char& c : windowsImagePathResult) {
    if (c == '/') {
      c = '\\';
    }
  }

  // Check if the path contains the word "shaders" and double backslashes if
  // true
  if (windowsImagePathResult.find("shaders") != std::string::npos) {
    std::string commandLine;
    for (char c : windowsImagePathResult) {
      if (c == '\\') {
        commandLine += "\\\\";
      } else {
        commandLine += c;
      }
    }
    // Change file extension to .bat if it ends with .sh
    size_t dotShPosition = commandLine.rfind(".sh");
    if (dotShPosition != std::string::npos &&
        dotShPosition + 3 == commandLine.length()) {
      commandLine.replace(dotShPosition, 3, ".bat");
    }
    windowsImagePathResult = commandLine;
  }

  // Remove leading ".\\" if present
  if (windowsImagePathResult.substr(0, 3) == "..\\.") {
    windowsImagePathResult = windowsImagePathResult.substr(3);
  }

  return windowsImagePathResult;
}
