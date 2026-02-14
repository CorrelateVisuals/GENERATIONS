#include <glm/glm.hpp>

#include "Library.h"

#include <chrono>
#include <numbers>
#include <random>

std::vector<float> Lib::generate_random_values(int amount, float min, float max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(min, max);
  const size_t count = static_cast<size_t>(amount);
  std::vector<float> randomValues(count);
  for (size_t i = 0; i < count; ++i) {
    randomValues[i] = dis(gen);
  }
  return randomValues;
}

double Lib::low_frequency_oscillator(double frequency) {
  using namespace std::chrono;
  static const auto start_time = high_resolution_clock::now();
  const auto time_elapsed =
      duration_cast<milliseconds>(high_resolution_clock::now() - start_time).count();
  constexpr double milliseconds_per_second = 1000.0;
  constexpr double full_rotation = 2.0;
  const double angle = time_elapsed * frequency * full_rotation * std::numbers::pi /
                       milliseconds_per_second;
  return 0.5 * (1 + std::sin(angle));
}

glm::vec2 Lib::smoothstep(const glm::vec2 &xy) {
  constexpr float startInput = 0.0f;
  constexpr float endInput = 1.0f;
  constexpr float minIncrease = -0.1f;
  constexpr float maxIncrease = 0.1f;

  const float tX = (xy.x - startInput) / (endInput - startInput);
  const float tY = (xy.y - startInput) / (endInput - startInput);

  const float smoothX = tX * tX * (3.0f - 2.0f * tX);
  const float smoothY = tY * tY * (3.0f - 2.0f * tY);

  glm::vec2 increase = glm::mix(
      glm::vec2(minIncrease), glm::vec2(maxIncrease), glm::vec2(smoothX, smoothY));
  return increase;
}

std::string Lib::upper_to_lower_case(std::string string) {
  std::string lower_case_string = "";
  for (char c : string) {
    if (std::isupper(c)) {
      c = std::tolower(c);
    }
    lower_case_string += c;
  }
  return lower_case_string;
}

std::string Lib::path(const std::string &linux_path) {
#ifdef _WIN32
  std::string converted_windows_path = "..\\" + linux_path;
  for (char &c : converted_windows_path) {
    if (c == '/') {
      c = '\\';
    }
  }
  if (converted_windows_path.substr(0, 3) == "..\\.") {
    converted_windows_path = converted_windows_path.substr(3);
  }
  return if_shader_compile(converted_windows_path);
#else
  return if_shader_compile(linux_path);
#endif
}

std::string Lib::if_shader_compile(std::string shader_path) {
  if (shader_path.find("shaders") == std::string::npos) {
    return shader_path;
  } else {
#ifdef _WIN32
  const std::string glslang_validator = "glslangValidator.exe -V ";
#else
    const std::string glslang_validator = "glslangValidator -V ";
#endif
    shader_path.insert(0, glslang_validator);
    return shader_path;
  }
}
