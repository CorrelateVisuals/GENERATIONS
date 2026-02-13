#include "io/Library.h"

#include <vector>

class Terrain {
public:
  struct Config {
    vec2_uint_fast16_t dimensions;
    float roughness;
    int octaves;
    float scale;
    float amplitude;
    float exponent;
    float frequency;
    float heightOffset;
  } config;

  Terrain(const Config &config);
  virtual ~Terrain() = default;

  std::vector<float> generatePerlinGrid();
  float linearInterpolationFunction(float a, float b, float t);
};
