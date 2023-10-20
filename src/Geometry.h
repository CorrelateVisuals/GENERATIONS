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

const enum ORIENTATION_ORDER { ROTATE_SCALE_TRANSLATE, ROTATE_TRANSLATE_SCALE };

class Vertex {
 public:
  union {
    glm::vec3 instancePosition;
    glm::vec4 instancePosition_16bytes;
  };
  union{
    glm::vec3 vertexPosition;
    glm::vec4 vertexPosition_16bytes;
  };
  union{
    glm::vec3 normal;
    glm::vec4 normal_16bytes;
  };
  union{
    glm::vec3 color;
    glm::vec4 color_16bytes;
  };
  union{
    glm::vec2 textureCoordinates;
    glm::ivec4 textureCoordinates_16bytes;
  };

  static std::vector<VkVertexInputBindingDescription> getBindingDescription();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions();

  bool operator==(const Vertex& other) const {
    return vertexPosition == other.vertexPosition && color == other.color &&
           textureCoordinates == other.textureCoordinates &&
           normal == other.normal;
  }
};

class Geometry : public Vertex {
 public:
  Geometry(const std::string& modelName = "");
  virtual ~Geometry() = default;
  std::vector<Vertex> allVertices;
  std::vector<Vertex> uniqueVertices;
  std::vector<uint32_t> indices;

  struct VertexBuffer : public CE::Buffer {
  } vertexBuffer;
  struct IndexBuffer : public CE::Buffer {
  } indexBuffer;

  void addVertexPosition(const glm::vec3& position);
  static std::vector<uint32_t> createGridPolygons(
      const std::vector<uint32_t>& vertices,
      uint32_t gridWidth);

 private:
  void loadModel(const std::string& modelName, Geometry& geometry);
  void transformModel(std::vector<Vertex>& vertices,
                      ORIENTATION_ORDER order,
                      const glm::vec3& degrees = glm::vec3(0.0f),
                      const glm::vec3& translationDistance = glm::vec3(0.0f),
                      float scale = 1.0f);
};
