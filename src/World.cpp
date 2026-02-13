#include "World.h"
#include "CapitalEngine.h"
#include "Geometry.h"
#include "Terrain.h"
#include "core/Log.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>

namespace {
constexpr glm::ivec2 GRID_SIZE = {50, 50};
constexpr uint_fast32_t NUMBER_OF_ALIVE_CELLS = 200;
constexpr float CELL_SIZE = 0.5f;

constexpr float TIMER_SPEED = 25.0f;
constexpr float WATER_THRESHOLD = 0.1f;
constexpr glm::vec4 LIGHT_POS = {0.0f, 20.0f, 20.0f, 0.0f};

constexpr float ZOOM_SPEED = 0.5f;
constexpr float PANNING_SPEED = 1.2f;
constexpr float FIELD_OF_VIEW = 40.0f;
constexpr float NEAR_CLIPPING = 0.1f;
constexpr float FAR_CLIPPING = 1000.0f;
constexpr glm::vec3 CAMERA_POSITION = {0.0f, 0.0f, 60.0f};

constexpr GEOMETRY_SHAPE cube = CE_CUBE;  
constexpr GEOMETRY_SHAPE rectangle = CE_RECTANGLE;
constexpr GEOMETRY_SHAPE sphere = CE_SPHERE;
}  // namespace

World::World(VkCommandBuffer& commandBuffer,
             const VkCommandPool& commandPool,
             const VkQueue& queue)
    : _grid(GRID_SIZE,
            NUMBER_OF_ALIVE_CELLS,
            CELL_SIZE,
            commandBuffer,
            commandPool,
            queue),
      _rectangle(rectangle, true, commandBuffer, commandPool, queue),
      _cube(sphere, false, commandBuffer, commandPool, queue),
      _ubo(LIGHT_POS, GRID_SIZE, WATER_THRESHOLD, CELL_SIZE),
      _camera(ZOOM_SPEED,
              PANNING_SPEED,
              FIELD_OF_VIEW,
              NEAR_CLIPPING,
              FAR_CLIPPING,
              CAMERA_POSITION),
      _time(TIMER_SPEED) {
  const float halfGridX = 0.5f * static_cast<float>(GRID_SIZE.x) * CELL_SIZE;
  const float halfGridY = 0.5f * static_cast<float>(GRID_SIZE.y) * CELL_SIZE;
  const float sceneRadius =
      std::sqrt(halfGridX * halfGridX + halfGridY * halfGridY);
  _camera.configureArcball(glm::vec3(0.0f, 0.0f, 0.0f), sceneRadius);

  Log::text("{ wWw }", "constructing World");
}

World::~World() {
  Log::text("{ wWw }", "destructing World");
}

std::vector<VkVertexInputBindingDescription>
World::Cell::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> description{
      {0, sizeof(Cell), VK_VERTEX_INPUT_RATE_INSTANCE},
      {1, sizeof(Shape::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return description;
}

std::vector<VkVertexInputAttributeDescription>
World::Cell::getAttributeDescription() {
  std::vector<VkVertexInputAttributeDescription> description{
      {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Cell, instancePosition))},
      {1, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Shape::Vertex, vertexPosition))},
      {2, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Shape::Vertex, normal))},
      {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Cell, color))},
      {4, 0, VK_FORMAT_R32G32B32A32_SINT,
       static_cast<uint32_t>(offsetof(Cell, states))}};
  return description;
};

World::Grid::Grid(vec2_uint_fast16_t gridSize,
                  uint_fast32_t aliveCells,
                  float cellSize,
                  VkCommandBuffer& commandBuffer,
                  const VkCommandPool& commandPool,
                  const VkQueue& queue)
    : size(gridSize),
      initialAliveCells(aliveCells),
      pointCount(size.x * size.y) {
  Terrain::Config terrainLayer1 = {.dimensions = size,
                                   .roughness = 0.4f,
                                   .octaves = 10,
                                   .scale = 1.1f,
                                   .amplitude = 5.0f,
                                   .exponent = 2.0f,
                                   .frequency = 2.0f,
                                   .heightOffset = 0.0f};
  Terrain terrain(terrainLayer1);

  Terrain::Config terrainLayer2 = {.dimensions = size,
                                   .roughness = 1.0f,
                                   .octaves = 10,
                                   .scale = 1.1f,
                                   .amplitude = 0.3f,
                                   .exponent = 1.0f,
                                   .frequency = 2.0f,
                                   .heightOffset = 0.0f};
  Terrain terrainSurface(terrainLayer2);

  std::vector<float> terrainPerlinGrid1 = terrain.generatePerlinGrid();
  std::vector<float> terrainPerlinGrid2 = terrainSurface.generatePerlinGrid();
  const float blendFactor = 0.5f;

  std::vector<bool> isAliveIndices(pointCount, false);
  std::vector<uint_fast32_t> aliveCellIndices =
      setCellsAliveRandomly(initialAliveCells);
  for (int aliveIndex : aliveCellIndices) {
    isAliveIndices[aliveIndex] = true;
  }

  const glm::vec4 red{1.0f, 0.0f, 0.0f, 1.0f};
  const glm::vec4 blue{0.0f, 0.0f, 1.0f, 1.0f};
  const glm::ivec4 alive{1, 0, 0, 0};
  const glm::ivec4 dead{-1, 0, 0, 0};

  const float startX = (size.x - 1) / -2.0f;
  const float startY = (size.y - 1) / -2.0f;
  for (uint_fast32_t i = 0; i < pointCount; ++i) {
    pointIDs[i] = i;

    float height = terrain.linearInterpolationFunction(
        terrainPerlinGrid1[i], terrainPerlinGrid2[i], blendFactor);
    coordinates[i] = {(startX + i % size.x), (startY + i / size.x), height};
    addVertexPosition(coordinates[i]);

    const bool isAlive = isAliveIndices[i];

    cells[i].instancePosition = {coordinates[i], isAlive ? cellSize : 0.0f};
    cells[i].color = isAlive ? blue : red;
    cells[i].states = isAlive ? alive : dead;
  }
  indices = createGridPolygons(pointIDs, static_cast<int>(size.x));
  createVertexBuffer(commandBuffer, commandPool, queue, uniqueVertices);
  createIndexBuffer(commandBuffer, commandPool, queue, indices);
}

std::vector<VkVertexInputAttributeDescription>
World::Grid::getAttributeDescription() {
  std::vector<VkVertexInputAttributeDescription> attributes{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Grid::Vertex, vertexPosition))}};
  return attributes;
}

std::vector<uint_fast32_t> World::Grid::setCellsAliveRandomly(
    uint_fast32_t numberOfCells) {
  const uint_fast32_t targetCount =
      std::min<uint_fast32_t>(numberOfCells,
                              static_cast<uint_fast32_t>(pointCount));

  std::vector<uint_fast32_t> cellIds(pointCount);
  std::iota(cellIds.begin(), cellIds.end(), 0);

  std::random_device random;
  std::mt19937 generate(random());
  std::shuffle(cellIds.begin(), cellIds.end(), generate);

  cellIds.resize(targetCount);
  std::sort(cellIds.begin(), cellIds.end());
  return cellIds;
}
