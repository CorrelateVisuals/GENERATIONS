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

  struct Shape : public Geometry {
    Shape(std::string shape,
          bool indices,
          VkCommandBuffer& commandBuffer,
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
    Light(glm::vec4 p) : position(p){};
  };

  class Camera {
   public:
    float zoomSpeed{};
    float panningSpeed{};
    float fieldOfView{};
    float nearClipping{};
    float farClipping{};
    glm::vec3 position{};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, -1.0f, 0.0f};

    Camera(float zoom,
           float pan,
           float fov,
           float near,
           float far,
           glm::vec3 pos)
        : zoomSpeed(zoom),
          panningSpeed(pan),
          fieldOfView(fov),
          nearClipping(near),
          farClipping(far),
          position(pos){};

    glm::mat4 setModel();
    glm::mat4 setView();
    glm::mat4 setProjection(const VkExtent2D& swapchainExtent);

   private:
    void update();
  };

  float speed{25.0f};
  Timer time{speed};

  Grid grid;
  Shape rectangle;
  Shape cube;

  Light light;

  Camera camera;
};

// float getForwardMovement(const glm::vec2& leftButtonDelta);
