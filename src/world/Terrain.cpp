#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include "Terrain.h"

#include <cmath>
#include <vector>

Terrain::Terrain(const Config &config) : config(config) {}

std::vector<float> Terrain::generate_perlin_grid() {
  uint_fast16_t point_count = config.dimensions.x * config.dimensions.y;
  std::vector<float> random_values(point_count);

  for (uint_fast16_t i = 0; i < point_count; i++) {
    glm::vec2 position(i % config.dimensions.x, i / config.dimensions.x);
    glm::vec2 scaled_position =
        position / glm::vec2(config.dimensions.x, config.dimensions.y);

    float total_noise = 0.0f;
    float frequency = config.frequency;
    float amplitude = config.amplitude;

    for (int octave = 0; octave < config.octaves; octave++) {
      total_noise += glm::perlin(scaled_position * config.scale * frequency) * amplitude;
      frequency *= 2.0f;
      amplitude *= config.roughness;
    }

    total_noise = pow(total_noise, config.exponent);

    random_values[i] = total_noise + config.height_offset;
  }
  return random_values;
}

float Terrain::linear_interpolation_function(float a, float b, float t) {
  return a * (1.0f - t) + b * t;
}
