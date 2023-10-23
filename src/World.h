#pragma once
#include "Geometry.h"
#include "Library.h"
#include "Timer.h"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

class Timer;

class World {
 public:
  World();
  ~World();

  Timer time{25.0f};

  struct Grid {
    const uint_fast32_t initialAliveCells = 5000;
    vec2_uint_fast16_t size = {100, 100};
  } grid;

  struct Landscape : public Geometry {
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  } landscape;

  struct Rectangle : public Geometry {
    Rectangle() : Geometry("Rectangle"){};
  } rectangle;

  struct Cube : public Geometry {
    Cube() : Geometry("SphereHR"){};
    const float size = 0.5f;
  } cube;

  struct alignas(16) Cell {
    glm::vec4 instancePosition;
    glm::vec4 vertexPosition;
    glm::vec4 normal;
    glm::vec4 color;
    glm::ivec4 states;

   //std::vector<VkVertexInputBindingDescription> getBindingDescription();
    
  } cell;
  struct PipelineShaderAccessDiscription {
    std::vector<VkVertexInputBindingDescription> binding{
        {0, sizeof(Cell), VK_VERTEX_INPUT_RATE_INSTANCE},
        {1, sizeof(Cube::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    std::vector<VkVertexInputAttributeDescription> attributes{
        {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
         static_cast<uint32_t>(offsetof(Cell, instancePosition))},
        {1, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
         static_cast<uint32_t>(offsetof(Cube::Vertex, vertexPosition))},
        {2, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
         static_cast<uint32_t>(offsetof(Cube::Vertex, normal))},
        {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
         static_cast<uint32_t>(offsetof(Cell, color))},
        {4, 0, VK_FORMAT_R32G32B32A32_SINT,
         static_cast<uint32_t>(offsetof(Cell, states))}};
  } cellBindings;

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
  std::vector<World::Cell> initializeGrid();
  UniformBufferObject updateUniforms(VkExtent2D& _swapchain);

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

  const glm::vec4 red{1.0f, 0.0f, 0.0f, 1.0f};
  const glm::vec4 blue{0.0f, 0.0f, 1.0f, 1.0f};
  const glm::ivec4 alive{1, 0, 0, 0};
  const glm::ivec4 dead{-1, 0, 0, 0};

 private:
  void updateCamera();
  std::vector<uint_fast32_t> setCellsAliveRandomly(uint_fast32_t numberOfCells);
  bool isCellIndexAlive(const std::vector<int>& aliveCells, int index);
  std::vector<float> generateLandscapeHeight();

  glm::mat4 setModel();
  glm::mat4 setView();
  glm::mat4 setProjection(VkExtent2D& swapchainExtent);
};

// float getForwardMovement(const glm::vec2& leftButtonDelta);
