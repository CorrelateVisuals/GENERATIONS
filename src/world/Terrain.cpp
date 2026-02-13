#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include "Terrain.h"

#include <cmath>
#include <vector>

Terrain::Terrain(const Config &config) : config(config) {}

std::vector<float> Terrain::generatePerlinGrid() {
  uint_fast16_t pointCount = config.dimensions.x * config.dimensions.y;
  std::vector<float> randomValues(pointCount);

  for (uint_fast16_t i = 0; i < pointCount; i++) {
    glm::vec2 position(i % config.dimensions.x, i / config.dimensions.x);
    glm::vec2 scaledPosition =
        position / glm::vec2(config.dimensions.x, config.dimensions.y);

    float totalNoise = 0.0f;
    float frequency = config.frequency;
    float amplitude = config.amplitude;

    for (int octave = 0; octave < config.octaves; octave++) {
      totalNoise += glm::perlin(scaledPosition * config.scale * frequency) * amplitude;
      frequency *= 2.0f;
      amplitude *= config.roughness;
    }

    totalNoise = pow(totalNoise, config.exponent);

    randomValues[i] = totalNoise + config.heightOffset;
  }
  return randomValues;
}

float Terrain::linearInterpolationFunction(float a, float b, float t) {
  return a * (1.0f - t) + b * t;
}
