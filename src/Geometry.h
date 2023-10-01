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

struct Geometry {
  struct Vertex {
    glm::vec3 instancePosition;
    glm::vec3 vertexPosition;
    glm::vec3 color;
    glm::vec2 textureCoordinates;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();

    bool operator==(const Vertex& other) const {
      return vertexPosition == other.vertexPosition && color == other.color &&
             textureCoordinates == other.textureCoordinates;
    }
  };
  std::string model;
  std::string modelPath;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  Geometry(const std::string& modelName = "") : model(modelName) {
    modelPath = Lib::path("assets/3D/" + model + ".obj");
  }
  ~Geometry() = default;

  void addVertexPosition(const glm::vec3& position) {
    vertices.push_back({glm::vec3(0.0f), position});
  }
};
