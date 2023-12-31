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
    : grid{commandBuffer, commandPool, queue},
      rectangle{commandBuffer, commandPool, queue},
      cube{commandBuffer, commandPool, queue} {
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

World::Grid::Grid(VkCommandBuffer& commandBuffer,
                  const VkCommandPool& commandPool,
                  const VkQueue& queue) {
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
    coorindates[i] = {(startX + i % size.x), (startY + i / size.x), height};
    addVertexPosition(coorindates[i]);

    const bool isAlive = isAliveIndices[i];

    cells[i].instancePosition = {coorindates[i],
                                 isAlive ? initialCellSize : 0.0f};
    cells[i].color = isAlive ? blue : red;
    cells[i].states = isAlive ? alive : dead;
  }
  indices = createGridPolygons(pointIDs, static_cast<int>(size.x));
  createVertexBuffer(commandBuffer, commandPool, queue, uniqueVertices);
  createIndexBuffer(commandBuffer, commandPool, queue, indices);
}

World::Rectangle::Rectangle(VkCommandBuffer& commandBuffer,
                            const VkCommandPool& commandPool,
                            const VkQueue& queue)
    : Geometry("Rectangle") {
  createVertexBuffer(commandBuffer, commandPool, queue, uniqueVertices);
  createIndexBuffer(commandBuffer, commandPool, queue, indices);
}

World::Cube::Cube(VkCommandBuffer& commandBuffer,
                  const VkCommandPool& commandPool,
                  const VkQueue& queue)
    : Geometry("Cube") {
  createVertexBuffer(commandBuffer, commandPool, queue, allVertices);
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

void World::Camera::update() {
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
    glm::vec3 cameraRight = glm::cross(front, up);

    glm::vec3 cameraUp = glm::cross(cameraRight, front);
    position -= panningSpeed * leftButtonDelta.x * cameraRight;
    position -= panningSpeed * leftButtonDelta.y * cameraUp;

    position += zoomSpeed * rightButtonDelta.x * front;
    position.z = std::max(position.z, 0.0f);
  }
  run = mousePositionChanged;
}

glm::mat4 World::Camera::setModel() {
  glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f),
                                glm::vec3(0.0f, 0.0f, 1.0f));
  return model;
}

glm::mat4 World::Camera::setView() {
  update();
  glm::mat4 view;
  view = glm::lookAt(position, position + front, up);
  return view;
}

glm::mat4 World::Camera::setProjection(const VkExtent2D& swapchainExtent) {
  glm::mat4 projection = glm::perspective(
      glm::radians(fieldOfView),
      swapchainExtent.width / static_cast<float>(swapchainExtent.height),
      nearClipping, farClipping);

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
