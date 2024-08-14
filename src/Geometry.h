#pragma once
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>

#include "BaseClasses.h"

#include <string>
#include <vector>

enum ORIENTATION_ORDER { CE_ROTATE_SCALE_TRANSLATE, CE_ROTATE_TRANSLATE_SCALE };
enum GEOMETRY_SHAPE {
  CE_RECTANGLE,
  CE_CUBE,
  CE_SPHERE,
  CE_SPHERE_HR,
  CE_TORUS
};

class Vertex {
 public:
  glm::vec3 instancePosition{};
  glm::vec3 vertexPosition{};
  glm::vec3 normal{};
  glm::vec3 color{};
  glm::vec2 textureCoordinates{};

  static std::vector<VkVertexInputBindingDescription> getBindingDescription();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescription();

  bool operator==(const Vertex& other) const {
    return vertexPosition == other.vertexPosition && color == other.color &&
           textureCoordinates == other.textureCoordinates &&
           normal == other.normal;
  }
};

class Geometry : public Vertex {
 public:
  Geometry() = default;
  Geometry(GEOMETRY_SHAPE shape);
  virtual ~Geometry() = default;
  std::vector<Vertex> allVertices{};
  std::vector<Vertex> uniqueVertices{};
  std::vector<uint32_t> indices{};

  CE::Buffer vertexBuffer;
  CE::Buffer indexBuffer;

  void addVertexPosition(const glm::vec3& position);
  static std::vector<uint32_t> createGridPolygons(
      const std::vector<uint32_t>& vertices,
      uint32_t gridWidth);

 protected:
  void createVertexBuffer(VkCommandBuffer& commandBuffer,
                          const VkCommandPool& commandPool,
                          const VkQueue& queue,
                          const std::vector<Vertex>& vertices);

  void createIndexBuffer(VkCommandBuffer& commandBuffer,
                         const VkCommandPool& commandPool,
                         const VkQueue& queue,
                         const std::vector<uint32_t>& indices);

 private:
  void loadModel(const std::string& modelName, Geometry& geometry);
  void transformModel(std::vector<Vertex>& vertices,
                      ORIENTATION_ORDER order,
                      const glm::vec3& degrees = glm::vec3(0.0f),
                      const glm::vec3& translationDistance = glm::vec3(0.0f),
                      float scale = 1.0f);
};

class Shape : public Geometry {
 public:
  Shape(GEOMETRY_SHAPE shape,
        bool hasIndices,
        VkCommandBuffer& commandBuffer,
        const VkCommandPool& commandPool,
        const VkQueue& queue);
};
