#include <vector>

class Terrain {
public:
    struct Config {
        uint_fast16_t width;
        uint_fast16_t height;
        float roughness;
        int octaves;
        float scale;
        float amplitude;
        float exponent;
        float frequency;
    };

    Terrain(const Config& _config);

    std::vector<float> generatePerlinGrid(size_t numPoints);
    float linearInterpolationFunction(float a, float b, float t);

private:
    Config config;
};