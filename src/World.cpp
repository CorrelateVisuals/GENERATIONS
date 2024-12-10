#include "World.h"
#include "CapitalEngine.h"
#include "Geometry.h"
#include "Terrain.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <random>

namespace {
constexpr glm::ivec2 GRID_SIZE = {250, 250};
constexpr uint_fast32_t NUMBER_OF_ALIVE_CELLS = 30000;
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
  std::vector<uint_fast32_t> CellIDs;
  CellIDs.reserve(numberOfCells);

  std::random_device random;
  std::mt19937 generate(random());
  std::uniform_int_distribution<int> distribution(
      0, static_cast<int>(pointCount) - 1);

  while (CellIDs.size() < numberOfCells) {
    int CellID = distribution(generate);
    if (std::find(CellIDs.begin(), CellIDs.end(), CellID) == CellIDs.end())
        [[likely]] {
      CellIDs.push_back(CellID);
    }
  }
  std::sort(CellIDs.begin(), CellIDs.end());
  return CellIDs;
}
