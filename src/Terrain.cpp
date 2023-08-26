#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include "Terrain.h"

#include <cmath>
#include <vector>

Terrain::Terrain(const Config& _config) : config(_config) {}

std::vector<float> Terrain::generatePerlinGrid() {
  size_t numPoints = config.width * config.height;
  std::vector<float> randomValues(numPoints);

  for (size_t i = 0; i < numPoints; i++) {
    glm::vec2 position(i % config.width, i / config.width);
    glm::vec2 scaledPosition =
        position / glm::vec2(config.width, config.height);

    float totalNoise = 0.0f;
    float frequency = config.frequency;  // Use the provided frequency
    float amplitude = config.amplitude;

    for (int octave = 0; octave < config.octaves; octave++) {
      totalNoise +=
          glm::perlin(scaledPosition * config.scale * frequency) * amplitude;
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
