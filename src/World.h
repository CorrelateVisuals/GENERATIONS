#pragma once
#include "Geometry.h"
#include "Library.h"
#include "Timer.h"
#include "Camera.h"

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

  struct Grid : public Geometry {
    vec2_uint_fast16_t size = {100, 100};
    uint_fast32_t initialAliveCells = 5000;
    const size_t pointCount{size.x * size.y};

    std::vector<uint32_t> pointIDs = std::vector<uint32_t>(pointCount);
    std::vector<glm::vec3> coordinates = std::vector<glm::vec3>(pointCount);
    std::vector<World::Cell> cells = std::vector<World::Cell>(pointCount);
    const float initialCellSize{0.5f};

    Grid(vec2_uint_fast16_t size,
         uint_fast32_t aliveCells,
         VkCommandBuffer& commandBuffer,
         const VkCommandPool& commandPool,
         const VkQueue& queue);
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();

   private:
    std::vector<uint_fast32_t> setCellsAliveRandomly(
        uint_fast32_t numberOfCells);
  };

  struct UniformBufferObject {
    glm::vec4 light{};
    glm::ivec2 gridXY{};
    float waterThreshold{};
    float cellSize{};
    alignas(16) glm::mat4 model{};
    alignas(16) glm::mat4 view{};
    alignas(16) glm::mat4 projection{};
  };

  class Light {
  public:
    glm::vec4 position;
    Light(glm::vec4 p) : position(p){};
  };

  Light light;
  Camera camera;
  Timer time;

  Grid grid;
  Shape rect;
  Shape cube;
};

// float getForwardMovement(const glm::vec2& leftButtonDelta);
