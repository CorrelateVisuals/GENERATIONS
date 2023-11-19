#pragma once
#include "Geometry.h"
#include "Library.h"
#include "Timer.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>
#include <vector>

class Timer;

class World {
 public:
  World(VkCommandBuffer& commandBuffer,
        const VkCommandPool& commandPool,
        const VkQueue& queue);
  ~World();

  float speed = 25.0f;
  Timer time{speed};

  struct alignas(16) Cell {
    glm::vec4 instancePosition;
    glm::vec4 vertexPosition;
    glm::vec4 normal;
    glm::vec4 color;
    glm::ivec4 states;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();
  };

  struct Grid : public Geometry {
    vec2_uint_fast16_t size = {100, 100};
    const uint_fast32_t initialAliveCells = 5000;
    const size_t pointCount{size.x * size.y};

    std::vector<uint32_t> pointIDs = std::vector<uint32_t>(pointCount);
    std::vector<glm::vec3> coorindates = std::vector<glm::vec3>(pointCount);
    std::vector<World::Cell> cells = std::vector<World::Cell>(pointCount);
    const float initialCellSize = 0.5f;

    Grid(VkCommandBuffer& commandBuffer,
         const VkCommandPool& commandPool,
         const VkQueue& queue);
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();

   private:
    std::vector<uint_fast32_t> setCellsAliveRandomly(
        uint_fast32_t numberOfCells);
  } grid;

  struct Rectangle : public Geometry {
    Rectangle(VkCommandBuffer& commandBuffer,
              const VkCommandPool& commandPool,
              const VkQueue& queue);
  } rectangle;

  struct Cube : public Geometry {
    Cube(VkCommandBuffer& commandBuffer,
         const VkCommandPool& commandPool,
         const VkQueue& queue);
  } cube;

  struct UniformBufferObject {
    glm::vec4 light;
    glm::ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
  };

 public:
  UniformBufferObject updateUniformBuferObject(
      const VkExtent2D& swapchainExtent);

 private:
  struct Camera {
    float zoomSpeed = 0.5f;
    float panningSpeed = 1.2f;
    const float fieldOfView = 40.0f;
    const float nearClipping = 0.1f;
    const float farClipping = 100.0f;
    glm::vec3 position{0.0f, 0.0f, 30.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, -1.0f, 0.0f};
  } camera;

  struct Light {
    glm::vec4 position{0.0f, 20.0f, 20.0f, 0.0f};
  } light;

 private:
  void updateCamera();

  glm::mat4 setModel();
  glm::mat4 setView();
  glm::mat4 setProjection(const VkExtent2D& swapchainExtent);
};

// float getForwardMovement(const glm::vec2& leftButtonDelta);
