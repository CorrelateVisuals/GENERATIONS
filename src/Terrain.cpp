#include "Terrain.h"

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include <iostream>
#include <random>
#include <vector>

std::vector<float> Terrain::generateSurfaceNoise(size_t numPoints) {
    std::vector<float> randomValues(numPoints);

    for (size_t i = 0; i < numPoints; i++) {
        glm::vec2 position(i % config.widthChars, i / config.widthChars);
        glm::vec2 scaledPosition = position / glm::vec2(config.widthChars, config.numRows);

        randomValues[i] = glm::perlin(scaledPosition * config.surfaceRoughness);
    }
    return randomValues;
}

void Terrain::generateGrid(std::vector<std::vector<size_t>>& grid) {
    float centerX = config.widthChars / 2.0f;
    float centerY = config.numRows / 2.0f;
    float maxRadius = std::min(centerX, centerY);

    for (size_t row = 0; row < config.numRows; ++row) {
        for (size_t col = 0; col < config.widthChars; ++col) {
            float distanceToCenter = glm::distance(glm::vec2(col, row), glm::vec2(centerX, centerY));

            float roundedFactor = 1.0f - (distanceToCenter / maxRadius * config.gridRoundness);
            roundedFactor = glm::clamp(roundedFactor, 0.0f, 1.0f); 

            size_t maxHeightWithRounding = static_cast<size_t>(config.maxHeight * roundedFactor);

            grid[row][col] = maxHeightWithRounding;
        }
    }
}

void Terrain::attachGrids(std::vector<std::vector<size_t>>& mainGrid,
                          const std::vector<std::vector<size_t>>& subGrid,
                          size_t rowOffset,
                          size_t colOffset) {
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

    std::vector<std::vector<size_t>> mainGrid(totalRows, std::vector<size_t>(totalCols, 0));
    std::vector<float> surfaceNoise = generateSurfaceNoise(totalRows * totalCols);
    std::vector<float> finalGrid(totalRows * totalCols);

    for (size_t i = 0; i < totalRows * totalCols; ++i) {
        size_t row = i / totalCols;
        size_t col = i % totalCols;
        size_t subRow = row % config.numRows;
        size_t subCol = col % config.widthChars;

        if (subCol == 0 && col > 0) {
            std::vector<std::vector<size_t>> subGrid(config.numRows, std::vector<size_t>(config.widthChars, 0));
            generateGrid(subGrid);
            attachGrids(mainGrid, subGrid, row - subRow, col - config.widthChars);
        }

        size_t height = std::min<size_t>(mainGrid[subRow][subCol], config.maxHeight);
        finalGrid[i] = static_cast<float>(height * config.heightMultiplyer + surfaceNoise[i]);
    }

    return finalGrid;
}
