#include <vector>

class Terrain {
 public:
  Terrain(){};
  ~Terrain(){};

  struct Configuration {
    size_t numRows = 50;
    size_t widthChars = 50;
    int maxHeight = 20;
    float heightMultiplyer = 0.25;
    size_t numGridsX = 10;
    size_t numGridsY = 10;
    size_t numGridPoints = (widthChars * numGridsX) * (numRows * numGridsY);
    float surfaceRoughness[2] = {0.0f, 0.75f};
    int surfaceHeightSteps = 10;
  } config;

 public:
  std::vector<float> generateTerrain();

private:
  std::vector<float> generateSurfaceNoise(size_t gridSize);
  void generateGrid(std::vector<std::vector<int>>& grid);
  void attachGrids(std::vector<std::vector<int>>& mainGrid,
                   const std::vector<std::vector<int>>& subGrid,
                   int rowOffset,
                   int colOffset);
};
