#include "Terrain_Generator.h"

#include <iostream>
#include <random>
#include <vector>

std::vector<float> World::generateSurfaceNoise() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(terrain.surfaceRoughness[0],
                                            terrain.surfaceRoughness[1]);
  std::vector<float> randomValues(terrain.numGridPoints);
  int heightSteps = terrain.surfaceHeightSteps;
  int offset = 0;
  for (size_t i = 0; i < terrain.numGridPoints; i++) {
    randomValues[i] = (dis(gen) * offset) / heightSteps;
    offset = (offset + 1) % heightSteps;
  }
  std::shuffle(randomValues.begin(), randomValues.end(), gen);
  return randomValues;
}

void World::generateGrid(int numRows,
                         int widthChars,
                         int maxHeight,
                         std::vector<std::vector<int>>& grid) {
  for (size_t row = 0; row < numRows; ++row) {
    for (size_t col = 0; col < widthChars; ++col) {
      int distanceToEdge = std::min(std::min(row, numRows - row - 1),
                                    std::min(col, widthChars - col - 1));
      grid[row][col] = distanceToEdge < maxHeight ? distanceToEdge : maxHeight;
    }
  }
  return;
}

void World::attachGrids(std::vector<std::vector<int>>& mainGrid,
                        const std::vector<std::vector<int>>& subGrid,
                        int rowOffset,
                        int colOffset) {
  size_t subRows = subGrid.size();
  size_t subCols = subGrid[0].size();

  for (size_t row = 0; row < subRows; ++row) {
    for (size_t col = 0; col < subCols; ++col) {
      mainGrid[row + rowOffset][col + colOffset] = subGrid[row][col];
    }
  }
  return;
}

std::vector<float> World::generateTerrain(Terrain terrain) {
  size_t totalRows = terrain.numRows * terrain.numGridsY;
  size_t totalCols = terrain.widthChars * terrain.numGridsX;

  std::vector<std::vector<int>> mainGrid(totalRows,
                                         std::vector<int>(totalCols, 0));

  for (size_t gridRow = 0; gridRow < terrain.numGridsY; ++gridRow) {
    for (size_t gridCol = 0; gridCol < terrain.numGridsX; ++gridCol) {
      std::vector<std::vector<int>> subGrid(
          terrain.numRows, std::vector<int>(terrain.widthChars, 0));
      generateGrid(terrain.numRows, terrain.widthChars, terrain.maxHeight,
                   subGrid);
      attachGrids(mainGrid, subGrid, gridRow * terrain.numRows,
                  gridCol * terrain.widthChars);
    }
  }

  std::vector<float> finalGrid;
  for (size_t row = 0; row < totalRows; ++row) {
    for (size_t col = 0; col < totalCols; ++col) {
      finalGrid.push_back(static_cast<float>(mainGrid[row][col]));
      std::cout << mainGrid[row][col] << " ";
    }
    std::cout << std::endl;
  }

  std::vector<float> surfaceNoise = generateSurfaceNoise();
  for (size_t i = 0; i < finalGrid.size(); i++) {
    finalGrid[i] += surfaceNoise[i];
  }

  return finalGrid;
}
