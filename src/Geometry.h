#pragma once
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>

#include "Library.h"

#include <string>
#include <vector>

const enum ORIENTATION_ORDER { ROTATE_SCALE_TRANSLATE, ROTATE_TRANSLATE_SCALE };

class Geometry {
 public:
  struct Vertex {
    glm::vec3 instancePosition;
    glm::vec3 vertexPosition;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 textureCoordinates;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();

    bool operator==(const Vertex& other) const {
      return vertexPosition == other.vertexPosition && color == other.color &&
             textureCoordinates == other.textureCoordinates &&
             normal == other.normal;
    }
  };
  std::vector<Vertex> allVertices;
  std::vector<Vertex> uniqueVertices;
  std::vector<uint32_t> indices;

  Geometry(const std::string& modelName = "") {
    if (!modelName.empty()) {
      loadModel(modelName, allVertices, uniqueVertices, indices,
                ORIENTATION_ORDER{ROTATE_SCALE_TRANSLATE},
                glm::vec3(90.0f, 180.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                1.0f);
    }
  }
  ~Geometry() = default;

  void addVertexPosition(const glm::vec3& position) {
    uniqueVertices.push_back({glm::vec3(0.0f), position});
  }

  static void loadModel(const std::string& modelName,
                        std::vector<Geometry::Vertex>& allVertices,
                        std::vector<Geometry::Vertex>& uniqueVertices,
                        std::vector<uint32_t>& indices,
                        ORIENTATION_ORDER order,
                        const glm::vec3& rotate,
                        const glm::vec3& translate,
                        float geoSize);

 private:
  static void transformModel(std::vector<Vertex>& vertices,
                             ORIENTATION_ORDER order,
                             const glm::vec3& degrees,
                             const glm::vec3& translationDistance,
                             float scale);
};
