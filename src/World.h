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
    const uint_fast32_t initialAliveCells = 5000;
    const size_t pointCount{size.x * size.y};

    std::vector<uint32_t> pointIDs = std::vector<uint32_t>(pointCount);
    std::vector<glm::vec3> coordinates = std::vector<glm::vec3>(pointCount);
    std::vector<World::Cell> cells = std::vector<World::Cell>(pointCount);
    const float initialCellSize{0.5f};

    Grid(VkCommandBuffer& commandBuffer,
         const VkCommandPool& commandPool,
         const VkQueue& queue);
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();

   private:
    std::vector<uint_fast32_t> setCellsAliveRandomly(
        uint_fast32_t numberOfCells);
  };

  struct Rectangle : public Geometry {
    Rectangle(VkCommandBuffer& commandBuffer,
              const VkCommandPool& commandPool,
              const VkQueue& queue);
  };

  struct Cube : public Geometry {
    Cube(VkCommandBuffer& commandBuffer,
         const VkCommandPool& commandPool,
         const VkQueue& queue);
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

  struct Light {
    glm::vec4 position;

    Light() = default;

    void initialize(glm::vec4 position) { this->position = position; }
  };

  class Camera {
   public:
    float zoomSpeed;
    float panningSpeed;
    const float fieldOfView;
    const float nearClipping;
    const float farClipping;
    glm::vec3 position;
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, -1.0f, 0.0f};

    Camera(float zoomSpeed = 0.5f,
           float panningSpeed = 1.2f,
           float fieldOfView = 40.0f,
           float nearClipping = 0.1f,
           float farClipping = 100.0f,
           glm::vec3 position = {0.0f, 0.0f, 30.0f})
        : zoomSpeed(zoomSpeed),
          panningSpeed(panningSpeed),
          fieldOfView(fieldOfView),
          nearClipping(nearClipping),
          farClipping(farClipping),
          position(position) {}

    glm::mat4 setModel();
    glm::mat4 setView();
    glm::mat4 setProjection(const VkExtent2D& swapchainExtent);

   private:
    void update();
  };

  float speed{25.0f};
  Timer time{speed};

  Grid grid;
  Rectangle rectangle;
  Cube cube;

  Light light;

  Camera camera{0.5f, 1.2f, 40.0f, 0.1f, 100.0f, {0.0f, 0.0f, 30.0f}};
};

// float getForwardMovement(const glm::vec2& leftButtonDelta);
