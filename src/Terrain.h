#include <vector>

class Terrain {
 public:
  Terrain(){};
  ~Terrain(){};

  struct Configuration {
    size_t numRows = 50;
    size_t widthChars = 50;
    size_t numGridsX = 10;
    size_t numGridsY = 10;
    float gridRoundness = 1.0f;

    int maxHeight = 4;
    float heightMultiplyer = 0.5;
    float surfaceRoughness = 10.0f;

    size_t numGridPoints = (widthChars * numGridsX) * (numRows * numGridsY);
  } config;

  std::vector<float> generateTerrain();

private:
  std::vector<float> generateSurfaceNoise(size_t gridSize);
  void generateGrid(std::vector<std::vector<size_t>>& grid);
  void attachGrids(std::vector<std::vector<size_t>>& mainGrid,
                   const std::vector<std::vector<size_t>>& subGrid,
                   size_t rowOffset,
                   size_t colOffset);
};
