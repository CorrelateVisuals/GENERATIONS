#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "CapitalEngine.h"
#include "Terrain.h"
#include "World.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <random>

World::World() {
  Log::logTitle();
  Log::text("{ wWw }", "constructing World");
}

World::~World() {
  Log::text("{ wWw }", "destructing World");
  Log::logFooter();
}

std::vector<VkVertexInputBindingDescription>
World::Cell::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
      {0, sizeof(Cell), VK_VERTEX_INPUT_RATE_INSTANCE}};
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
World::Cell::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
      {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Cell, instancePosition))},
      {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Cell, color))},
      {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Cell, size))},
      {3, 0, VK_FORMAT_R32G32B32A32_SINT,
       static_cast<uint32_t>(offsetof(Cell, states))},
  };
  return attributeDescriptions;
}

std::vector<VkVertexInputBindingDescription>
World::Rectangle::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
      {0, sizeof(Rectangle), VK_VERTEX_INPUT_RATE_VERTEX}};
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
World::Rectangle::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Rectangle, pos))},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Rectangle, color))},
      {2, 0, VK_FORMAT_R32G32_SFLOAT,
       static_cast<uint32_t>(offsetof(Rectangle, texCoord))}};
  return attributeDescriptions;
}

std::vector<VkVertexInputBindingDescription>
World::Landscape::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
      {0, sizeof(Landscape), VK_VERTEX_INPUT_RATE_VERTEX}};
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
World::Landscape::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
      {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Landscape, position))}};
  return attributeDescriptions;
}

std::vector<World::Cell> World::initializeCells() {
  const uint_fast16_t width{grid.XY[0]};
  const uint_fast16_t height{grid.XY[1]};
  const uint_fast32_t numGridPoints{width * height};
  const uint_fast32_t numAliveCells{grid.cellsAlive};
  const float gap{0.6f};
  std::array<float, 4> size{geo.cube.size};

  if (numAliveCells > numGridPoints) {
    throw std::runtime_error(
        "\n!ERROR! Number of alive cells exceeds number of grid "
        "points");
  }

  std::vector<World::Cell> cells(numGridPoints);
  std::vector<bool> isAliveIndices(numGridPoints, false);

  std::vector<uint_fast32_t> aliveCellIndices =
      setCellsAliveRandomly(grid.cellsAlive);
  for (int aliveIndex : aliveCellIndices) {
    isAliveIndices[aliveIndex] = true;
  }

  std::vector<float> landscapeHeight = generateLandscapeHeight();

  float startX = -((width - 1) * gap) / 2.0f;
  float startY = -((height - 1) * gap) / 2.0f;

  std::vector<uint32_t> tempIndices;

  for (uint_fast32_t i = 0; i < numGridPoints; ++i) {
    const uint_fast16_t x = static_cast<uint_fast16_t>(i % width);
    const uint_fast16_t y = static_cast<uint_fast16_t>(i / width);
    const float posX = startX + x * gap;
    const float posY = startY + y * gap;

    const std::array<float, 4> position = {posX, posY, landscapeHeight[i],
                                           1.0f};
    const bool isAlive = isAliveIndices[i];

    const std::array<float, 4>& color = isAlive ? blue : red;
    const std::array<int, 4>& state = isAlive ? alive : dead;

    cells[i] = {position, color, size, state};

    tempIndices.push_back(i);
    landscapeVertices.push_back({position});
  }

  landscapeIndices =
      Lib::createGridPolygons(tempIndices, static_cast<int>(grid.XY[0]));

  return cells;
}

std::vector<uint_fast32_t> World::setCellsAliveRandomly(
    uint_fast32_t numberOfCells) {
  std::vector<uint_fast32_t> CellIDs;
  CellIDs.reserve(numberOfCells);

  std::random_device random;
  std::mt19937 generate(random());
  std::uniform_int_distribution<int> distribution(0,
                                                  grid.XY[0] * grid.XY[1] - 1);

  while (CellIDs.size() < numberOfCells) {
    int CellID = distribution(generate);
    if (std::find(CellIDs.begin(), CellIDs.end(), CellID) == CellIDs.end()) {
      CellIDs.push_back(CellID);
    }
  }
  std::sort(CellIDs.begin(), CellIDs.end());
  return CellIDs;
}

bool World::isIndexAlive(const std::vector<int>& aliveCells, int index) {
  return std::find(aliveCells.begin(), aliveCells.end(), index) !=
         aliveCells.end();
}

std::vector<float> World::generateLandscapeHeight() {
  Terrain::Config terrainLayer1 = {.width = grid.XY[0],
                                   .height = grid.XY[1],
                                   .roughness = 0.4f,
                                   .octaves = 10,
                                   .scale = 1.1f,
                                   .amplitude = 5.0f,
                                   .exponent = 2.0f,
                                   .frequency = 2.0f,
                                   .heightOffset = 0.0f};
  Terrain terrain(terrainLayer1);

  Terrain::Config terrainLayer2 = {.width = grid.XY[0],
                                   .height = grid.XY[1],
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

World::UniformBufferObject World::updateUniforms(VkExtent2D& _swapChainExtent) {
  UniformBufferObject uniformObject{
      .light = light.position,
      .gridXY = {static_cast<uint32_t>(grid.XY[0]),
                 static_cast<uint32_t>(grid.XY[1])},
      .waterThreshold = 0.1f,
      .cellSize = geo.cube.size,
      .model = setModel(),
      .view = setView(),
      .projection = setProjection(_swapChainExtent)};
  return uniformObject;
}

template <>
struct std::hash<World::Rectangle> {
  size_t operator()(const World::Rectangle& vertex) const {
    return ((std::hash<glm::vec3>()(vertex.pos) ^
             (std::hash<glm::vec3>()(vertex.color) << 1)) >>
            1) ^
           (std::hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};

void World::loadModel() {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        MODEL_PATH.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Rectangle, uint32_t> uniqueVertices{};

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Rectangle vertex{};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      Log::text(" ", vertex.pos.x, vertex.texCoord.x, vertex.color.x);
      Log::text(" ", vertex.pos.y, vertex.texCoord.y, vertex.color.y);
      Log::text(" ", vertex.pos.z, vertex.color.z);

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] =
            static_cast<uint32_t>(rectangleVertices.size());
        rectangleVertices.push_back(vertex);
      }

      rectangleIndices.push_back(uniqueVertices[vertex]);
      Log::text(" ", uniqueVertices[vertex]);
    }
  }
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

glm::mat4 World::setProjection(VkExtent2D& swapChainExtent) {
  glm::mat4 projection = glm::perspective(
      glm::radians(camera.fieldOfView),
      swapChainExtent.width / static_cast<float>(swapChainExtent.height),
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
