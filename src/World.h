#pragma once
#include "Geometry.h"
#include "Timer.h"

#include <algorithm>
#include <array>
#include <vector>

class Timer;

class World {
 public:
  World();
  ~World();

  Timer time{5.0f};

  struct Grid {
    uint_fast32_t cellsAlive = 1250;
    std::array<uint_fast16_t, 2> XY = {100, 50};
  } grid;

  struct Rectangle : Geometry {
    Rectangle() : Geometry("Rectangle"){};
  } rectangle;

  struct Cube : Geometry {
    Cube() : Geometry("Cube"){};
    float size = 0.3f;
  } cube;

  struct Landscape : Geometry {
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  } landscape;

  struct Cell {
    glm::vec4 instancePosition;
    glm::vec4 color;
    glm::vec4 size;
    glm::ivec4 states;
    glm::vec4 vertexPosition;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  };

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
  std::vector<World::Cell> initializeCells();
  UniformBufferObject updateUniforms(VkExtent2D& _swapChain);
  void loadModel(const std::string& modelPath,
                 std::vector<Geometry::Vertex>& vertices,
                 std::vector<uint32_t>& indices,
                 const glm::vec3& rotate,
                 const glm::vec3& translate,
                 float geoSize);

  void loadModelVertices(const std::string& modelPath,
                         std::vector<Geometry::Vertex>& vertices,
                         std::vector<uint32_t>& indices,
                         const glm::vec3& rotate,
                         const glm::vec3& translate,
                         float geoSize);

  void transformModel(auto& vertices,
                      ORIENTATION_ORDER order,
                      const glm::vec3& degrees,
                      const glm::vec3& translationDistance,
                      float scale);

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
    glm::vec4 position{0.0f, 0.0f, 60.0f, 0.0f};
  } light;

  const glm::vec4 red{1.0f, 0.0f, 0.0f, 1.0f};
  const glm::vec4 blue{0.0f, 0.0f, 1.0f, 1.0f};
  const glm::ivec4 alive{1, 0, 0, 0};
  const glm::ivec4 dead{-1, 0, 0, 0};

  void updateCamera();
  // float getForwardMovement(const glm::vec2& leftButtonDelta);
  std::vector<uint_fast32_t> setCellsAliveRandomly(uint_fast32_t numberOfCells);
  bool isIndexAlive(const std::vector<int>& aliveCells, int index);

  std::vector<float> generateLandscapeHeight();

  glm::mat4 setModel();
  glm::mat4 setView();
  glm::mat4 setProjection(VkExtent2D& swapChainExtent);
};
