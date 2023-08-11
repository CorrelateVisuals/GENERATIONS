#include "Terrain.h"

#include <iostream>
#include <random>
#include <vector>

std::vector<float> Terrain::generateSurfaceNoise( size_t gridPoints ) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(config.surfaceRoughness[0],
                                            config.surfaceRoughness[1]);
  std::vector<float> randomValues(gridPoints);
  int heightSteps = config.surfaceHeightSteps;
  int offset = 0;
  for (size_t i = 0; i < gridPoints; i++) {
    randomValues[i] = (dis(gen) * offset) / heightSteps;
    offset = (offset + 1) % heightSteps;
  }
  std::shuffle(randomValues.begin(), randomValues.end(), gen);
  return randomValues;
}

void Terrain::generateGrid(std::vector<std::vector<int>>& grid) {
  for (size_t row = 0; row < config.numRows; ++row) {
    for (size_t col = 0; col < config.widthChars; ++col) {
      int distanceToEdge = std::min(std::min(row, config.numRows - row - 1),
                                    std::min(col, config.widthChars - col - 1));
      grid[row][col] =
          distanceToEdge < config.maxHeight ? distanceToEdge : config.maxHeight;
    }
  }
  return;
}

void Terrain::attachGrids(std::vector<std::vector<int>>& mainGrid,
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

std::vector<float> Terrain::generateTerrain() {
  size_t totalRows = config.numRows * config.numGridsY;
  size_t totalCols = config.widthChars * config.numGridsX;

  std::vector<std::vector<int>> mainGrid(totalRows,
                                         std::vector<int>(totalCols, 0));

  for (size_t gridRow = 0; gridRow < config.numGridsY; ++gridRow) {
    for (size_t gridCol = 0; gridCol < config.numGridsX; ++gridCol) {
      std::vector<std::vector<int>> subGrid(
          config.numRows, std::vector<int>(config.widthChars, 0));
      generateGrid(subGrid);
      attachGrids(mainGrid, subGrid, gridRow * config.numRows,
                  gridCol * config.widthChars);
    }
  }

  std::vector<float> finalGrid;
  for (size_t row = 0; row < totalRows; ++row) {
    for (size_t col = 0; col < totalCols; ++col) {
      finalGrid.push_back(static_cast<float>(mainGrid[row][col]));
    }
  }

  std::vector<float> surfaceNoise = generateSurfaceNoise( finalGrid.size() );

  std::cout << surfaceNoise.size() << " " << finalGrid.size();
  for (size_t i = 0; i < finalGrid.size(); i++) {
    finalGrid[i] *= config.heightMultiplyer;
    finalGrid[i] += surfaceNoise[i];
  }

  return finalGrid;
}
