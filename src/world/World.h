#pragma once
#include "Camera.h"
#include "Geometry.h"
#include "io/Library.h"
#include "core/Timer.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>
#include <vector>

class World {
 public:
  World(VkCommandBuffer& commandBuffer,
        const VkCommandPool& commandPool,
        const VkQueue& queue);
  ~World();

  struct alignas(16) Cell {
    glm::vec4 instancePosition{};
    glm::vec4 vertexPosition{};
    glm::vec4 normal{};
    glm::vec4 color{};
    glm::ivec4 states{};

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();
  };

  struct UniformBufferObject {
    glm::vec4 light;
    glm::ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    alignas(16) ModelViewProjection mvp{};

    UniformBufferObject(glm::vec4 l, glm::ivec2 xy, float w, float s)
        : light(l), gridXY(xy), waterThreshold(w), cellSize(s){};
  };

  struct Grid : public Geometry {
    vec2_uint_fast16_t size;
    const uint_fast32_t initialAliveCells;
    const size_t pointCount;

    std::vector<uint32_t> pointIDs = std::vector<uint32_t>(pointCount);
    std::vector<glm::vec3> coordinates = std::vector<glm::vec3>(pointCount);
    std::vector<World::Cell> cells = std::vector<World::Cell>(pointCount);

    Grid(vec2_uint_fast16_t gridSize,
         uint_fast32_t aliveCells,
         float cellSize,
         VkCommandBuffer& commandBuffer,
         const VkCommandPool& commandPool,
         const VkQueue& queue);
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();

   private:
    std::vector<uint_fast32_t> setCellsAliveRandomly(
        uint_fast32_t numberOfCells);
  };

  Grid _grid;
  Shape _rectangle;
  Shape _cube;

  UniformBufferObject _ubo;
  Camera _camera;
  Timer _time;
};
