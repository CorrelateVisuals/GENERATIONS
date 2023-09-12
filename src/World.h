#pragma once
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Timer.h"

#include <array>
#include <vector>

class Timer;

class World {
 public:
  World();
  ~World();

  Timer time{30.0f};

  struct Grid {
    uint_fast32_t cellsAlive = 5000;
    std::array<uint_fast16_t, 2> XY = {200, 100};
  } grid;

  struct Geometry {
    struct Cube {
      const uint32_t vertexCount{36};
      const float size{0.1f};
    } cube;
    struct Tile {
      const uint32_t vertexCount{54};
    } tile;
    struct Water {
      const uint32_t vertexCount{6};
    } water;
    struct Texture {
      const uint32_t vertexCount{6};
    } texture;
  } geo;

  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescription() {
      std::vector<VkVertexInputBindingDescription> bindingDescriptions{
          {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
      return bindingDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions() {
      std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
          {0, 0, VK_FORMAT_R32G32_SFLOAT,
           static_cast<uint32_t>(offsetof(Vertex, pos))},
          {1, 0, VK_FORMAT_R32G32B32_SFLOAT,
           static_cast<uint32_t>(offsetof(Vertex, color))},
          {2, 0, VK_FORMAT_R32G32_SFLOAT,
           static_cast<uint32_t>(offsetof(Vertex, texCoord))}};
      return attributeDescriptions;
    }
  };

  struct Cell {
    std::array<float, 4> position;
    std::array<float, 4> color;
    std::array<float, 4> size;
    std::array<int, 4> states;
    std::array<float, 4> tileSidesHeight;
    std::array<float, 4> tileCornersHeight;
    std::array<float, 4> textureCoords;
  };

  struct UniformBufferObject {
    std::array<float, 4> light;
    std::array<uint32_t, 2> gridXY;
    float waterThreshold;
    float cellSize;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  };

 public:
  std::vector<World::Cell> initializeCells();
  UniformBufferObject updateUniforms(VkExtent2D& _swapChain);
  static std::vector<VkVertexInputAttributeDescription>
  getCellAttributeDescriptions();
  static std::vector<VkVertexInputBindingDescription>
  getCellBindingDescriptions(VkVertexInputRate inputRate);

 private:
  struct Camera {
    float zoomSpeed = 0.5f;
    float panningSpeed = 1.2f;
    const float fieldOfView = 60.0f;
    const float nearClipping = 0.1f;
    const float farClipping = 200.0f;
    glm::vec3 position{0.0f, 0.0f, 30.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, -1.0f, 0.0f};
  } camera;

  struct Light {
    std::array<float, 4> position{0.0f, 0.0f, 200.0f, 0.0f};
  } light;

  inline static const std::array<float, 4> red{1.0f, 0.0f, 0.0f, 1.0f};
  inline static const std::array<float, 4> blue{0.0f, 0.0f, 1.0f, 1.0f};
  inline static const std::array<int, 4> alive{1, 0, 0, 0};
  inline static const std::array<int, 4> dead{-1, 0, 0, 0};

  void updateCamera();
  // float getForwardMovement(const glm::vec2& leftButtonDelta);
  std::vector<uint_fast32_t> setCellsAliveRandomly(uint_fast32_t numberOfCells);
  bool isIndexAlive(const std::vector<int>& aliveCells, int index);

  glm::mat4 setModel();
  glm::mat4 setView();
  glm::mat4 setProjection(VkExtent2D& swapChainExtent);
};
