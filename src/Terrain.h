#include <vector>

class Terrain {
 public:
  Terrain(){};
  ~Terrain(){};

  struct Configuration {
    size_t numRows = 8;
    size_t widthChars = 8;
    int maxHeight = 3;
    size_t numGridsX = 2;
    size_t numGridsY = 2;
    size_t numGridPoints = (numRows * widthChars) * (numGridsX * numGridsY);
    float surfaceRoughness[2] = {0.0f, 0.5f};
    int surfaceHeightSteps = 4;
  } config;

 public:
  std::vector<float> generateTerrain();

 private:
  std::vector<float> generateSurfaceNoise();
  void generateGrid(std::vector<std::vector<int>>& grid);
  void attachGrids(std::vector<std::vector<int>>& mainGrid,
                   const std::vector<std::vector<int>>& subGrid,
                   int rowOffset,
                   int colOffset);
};
