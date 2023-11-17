#include "World.h"
#include "CapitalEngine.h"
#include "Terrain.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <random>

World::World(VkCommandBuffer& commandBuffer,
             const VkCommandPool& commandPool,
             const VkQueue& queue)
    : cube{commandBuffer, commandPool, queue} {
  Log::text("{ wWw }", "constructing World");
}

World::~World() {
  Log::text("{ wWw }", "destructing World");
}

std::vector<VkVertexInputBindingDescription>
World::Cell::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> description{
      {0, sizeof(Cell), VK_VERTEX_INPUT_RATE_INSTANCE},
      {1, sizeof(Cube::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return description;
}

std::vector<VkVertexInputAttributeDescription>
World::Cell::getAttributeDescription() {
  std::vector<VkVertexInputAttributeDescription> description{
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
  return description;
};

std::vector<VkVertexInputAttributeDescription>
World::Landscape::getAttributeDescription() {
  std::vector<VkVertexInputAttributeDescription> attributes{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Landscape::Vertex, vertexPosition))}};
  return attributes;
}

std::vector<World::Cell> World::initializeGrid() {
  const uint_fast32_t numAliveCells{grid.initialAliveCells};

  if (numAliveCells > grid.numPoints) {
    throw std::runtime_error(
        "\n!ERROR! Number of alive cells exceeds number of grid "
        "points");
  }

  std::vector<World::Cell> cells(grid.numPoints);
  std::vector<bool> isAliveIndices(grid.numPoints, false);
  std::vector<float> landscapeHeight = generateLandscapeHeight();
  std::vector<uint32_t> tempIndices(grid.numPoints);

  std::vector<uint_fast32_t> aliveCellIndices =
      setCellsAliveRandomly(grid.initialAliveCells);
  for (int aliveIndex : aliveCellIndices) {
    isAliveIndices[aliveIndex] = true;
  }

  float startX = (grid.size.x - 1) / -2.0f;
  float startY = (grid.size.y - 1) / -2.0f;
  for (uint_fast32_t i = 0; i < grid.numPoints; ++i) {
    const float posX = startX + static_cast<uint_fast16_t>(i % grid.size.x);
    const float posY = startY + static_cast<uint_fast16_t>(i / grid.size.x);
    const bool isAlive = isAliveIndices[i];

    cells[i].instancePosition = {posX, posY, landscapeHeight[i],
                                 isAlive ? cube.size : 0.0f};
    cells[i].color = isAlive ? blue : red;
    cells[i].states = isAlive ? alive : dead;

    tempIndices[i] = i;
    landscape.addVertexPosition(glm::vec3(posX, posY, landscapeHeight[i]));
  }
  landscape.indices =
      Geometry::createGridPolygons(tempIndices, static_cast<int>(grid.size.x));

  return cells;
}

std::vector<uint_fast32_t> World::setCellsAliveRandomly(
    uint_fast32_t numberOfCells) {
  std::vector<uint_fast32_t> CellIDs;
  CellIDs.reserve(numberOfCells);

  std::random_device random;
  std::mt19937 generate(random());
  std::uniform_int_distribution<int> distribution(
      0, grid.size.x * grid.size.y - 1);

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

std::vector<float> World::generateLandscapeHeight() {
  Terrain::Config terrainLayer1 = {.width = grid.size.x,
                                   .height = grid.size.y,
                                   .roughness = 0.4f,
                                   .octaves = 10,
                                   .scale = 1.1f,
                                   .amplitude = 5.0f,
                                   .exponent = 2.0f,
                                   .frequency = 2.0f,
                                   .heightOffset = 0.0f};
  Terrain terrain(terrainLayer1);

  Terrain::Config terrainLayer2 = {.width = grid.size.x,
                                   .height = grid.size.y,
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

  std::vector<float> landscapeHeight(terrainPerlinGrid1.size());
  for (size_t i = 0; i < landscapeHeight.size(); i++) {
    float blendFactor = 0.5f;
    landscapeHeight[i] = terrain.linearInterpolationFunction(
        terrainPerlinGrid1[i], terrainPerlinGrid2[i], blendFactor);
  }
  return landscapeHeight;
}

World::UniformBufferObject World::updateUniformBuferObject(
    const VkExtent2D& swapchainExtent) {
  UniformBufferObject uniformObject{
      .light = light.position,
      .gridXY = {static_cast<uint32_t>(grid.size.x),
                 static_cast<uint32_t>(grid.size.y)},
      .waterThreshold = 0.1f,
      .cellSize = cube.size,
      .model = setModel(),
      .view = setView(),
      .projection = setProjection(swapchainExtent)};
  return uniformObject;
}

void World::updateCamera() {
  glm::vec2 buttonType[3]{};
  constexpr uint_fast8_t left = 0;
  constexpr uint_fast8_t right = 1;
  constexpr uint_fast8_t middle = 2;
  bool mousePositionChanged = false;
  static bool run = false;

  for (uint_fast8_t i = 0; i < 3; ++i) {
    buttonType[i] = Window::get().mouse.buttonDown[i].position -
                    Window::get().mouse.previousButtonDown[i].position;

    if (Window::get().mouse.buttonDown[i].position !=
        Window::get().mouse.previousButtonDown[i].position) {
      mousePositionChanged = true;
      Window::get().mouse.previousButtonDown[i].position =
          Window::get().mouse.buttonDown[i].position;
    }
  }
  if (mousePositionChanged) {
    run = mousePositionChanged;
  }

  if (run) {
    glm::vec2 leftButtonDelta = buttonType[left];
    glm::vec2 rightButtonDelta = buttonType[right];
    glm::vec2 middleButtonDelta = buttonType[middle];
    glm::vec3 cameraRight = glm::cross(camera.front, camera.up);

    glm::vec3 cameraUp = glm::cross(cameraRight, camera.front);
    camera.position -= camera.panningSpeed * leftButtonDelta.x * cameraRight;
    camera.position -= camera.panningSpeed * leftButtonDelta.y * cameraUp;

    camera.position += camera.zoomSpeed * rightButtonDelta.x * camera.front;
    camera.position.z = std::max(camera.position.z, 0.0f);
  }
  run = mousePositionChanged;
}

glm::mat4 World::setModel() {
  glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f),
                                glm::vec3(0.0f, 0.0f, 1.0f));
  return model;
}

glm::mat4 World::setView() {
  updateCamera();
  glm::mat4 view;
  view =
      glm::lookAt(camera.position, camera.position + camera.front, camera.up);
  return view;
}

glm::mat4 World::setProjection(const VkExtent2D& swapchainExtent) {
  glm::mat4 projection = glm::perspective(
      glm::radians(camera.fieldOfView),
      swapchainExtent.width / static_cast<float>(swapchainExtent.height),
      camera.nearClipping, camera.farClipping);

  projection[1][1] *= -1;  // flip y axis
  projection[0][0] *= -1;  // flip x axis

  return projection;
};

// void World::updateCamera() {
//     glm::vec2 buttonType[3]{};
//     constexpr uint_fast8_t left = 0;
//     constexpr uint_fast8_t right = 1;
//     constexpr uint_fast8_t middle = 2;
//     bool mousePositionChanged = false;
//     static bool run = false;
//
//     for (uint_fast8_t i = 0; i < 3; ++i) {
//         buttonType[i] = Window::get().mouse.buttonDown[i].position -
//             Window::get().mouse.previousButtonDown[i].position;
//
//         if (Window::get().mouse.buttonDown[i].position !=
//             Window::get().mouse.previousButtonDown[i].position) {
//             mousePositionChanged = true;
//             Window::get().mouse.previousButtonDown[i].position =
//                 Window::get().mouse.buttonDown[i].position;
//         }
//     }
//     if (mousePositionChanged) {
//         run = mousePositionChanged;
//     }
//
//     if (run) {
//         glm::vec2 leftButtonDelta = buttonType[left];
//         glm::vec2 rightButtonDelta = buttonType[right];
//         glm::vec2 middleButtonDelta = buttonType[middle];
//         glm::vec3 cameraRight = glm::cross(camera.front, camera.up);
//
//          glm::vec2 absDelta = glm::abs(leftButtonDelta);
//          constexpr float rotationSpeed = 0.4f * glm::pi<float>() / 180.0f;
//          glm::vec2 rotationDelta = rotationSpeed * leftButtonDelta;
//          glm::mat4 rotationMatrix(1.0f);
//
//          if (absDelta.y > absDelta.x) {
//            rotationMatrix =
//                glm::rotate(rotationMatrix, -rotationDelta.y, cameraRight);
//          } else if (absDelta.x > absDelta.y) {
//            rotationMatrix = glm::rotate(rotationMatrix, rotationDelta.x,
//            camera.up);
//          }
//          camera.front = glm::normalize(
//              glm::vec3(rotationMatrix * glm::vec4(camera.front, 0.0f)));
//          camera.up =
//              glm::normalize(glm::vec3(rotationMatrix * glm::vec4(camera.up,
//              0.0f)));
//
//            float movementSpeed = getForwardMovement(leftButtonDelta);
//            camera.position += movementSpeed * camera.front;
//         constexpr float panningSpeed = 1.3f;
//         glm::vec3 cameraUp = glm::cross(cameraRight, camera.front);
//         camera.position -= panningSpeed * rightButtonDelta.x * cameraRight;
//         camera.position -= panningSpeed * rightButtonDelta.y * cameraUp;
//
//         constexpr float zoomSpeed = 0.5f;
//         camera.position += zoomSpeed * middleButtonDelta.x * camera.front;
//         camera.position.z = std::max(camera.position.z, 0.0f);
//     }
//     run = mousePositionChanged;
// }

// float World::getForwardMovement(const glm::vec2& leftButtonDelta) {
//   static bool leftMouseButtonDown = false;
//   static float forwardMovement = 0.0f;
//   float leftButtonDeltaLength = glm::length(leftButtonDelta);
//
//   if (leftButtonDeltaLength > 0.0f) {
//     if (!leftMouseButtonDown) {
//       leftMouseButtonDown = true;
//       forwardMovement = 0.0f;
//     }
//     constexpr float maxSpeed = 0.1f;
//     constexpr float acceleration = 0.01f;
//
//     // Calculate the speed based on the distance from the center
//     float normalizedDeltaLength = glm::clamp(leftButtonDeltaLength,
//     0.0f, 1.0f); float targetSpeed =
//         glm::smoothstep(0.0f, maxSpeed, 1.0f - normalizedDeltaLength);
//     forwardMovement += acceleration * (targetSpeed - forwardMovement);
//     forwardMovement = glm::clamp(forwardMovement, 0.0f, maxSpeed);
//   } else {
//     leftMouseButtonDown = false;
//     forwardMovement = 0.0f;
//   }
//   return forwardMovement;
// }
