#include "asset_io/Library.h"

#include <vector>

class Terrain {
public:
  struct Config {
    Vec2UintFast16 dimensions;
    float roughness;
    int octaves;
    float scale;
    float amplitude;
    float exponent;
    float frequency;
    float height_offset;
  };

  Terrain(const Config &config);
  virtual ~Terrain() = default;

  std::vector<float> generate_perlin_grid();
  float linear_interpolation_function(float a, float b, float t);

private:
  Config config;
};
