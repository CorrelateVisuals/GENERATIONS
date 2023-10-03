#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Geometry.h"

#include <iostream>

std::vector<VkVertexInputBindingDescription>
Geometry::Vertex::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions{
      {0, sizeof(Geometry::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
Geometry::Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Geometry::Vertex, vertexPosition))},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Geometry::Vertex, color))},
      {2, 0, VK_FORMAT_R32G32_SFLOAT,
       static_cast<uint32_t>(offsetof(Geometry::Vertex, textureCoordinates))}};
  return attributeDescriptions;
}

template <>
struct std::hash<Geometry::Vertex> {
  size_t operator()(const Geometry::Vertex& vertex) const {
    return ((std::hash<glm::vec3>()(vertex.instancePosition) ^
             (std::hash<glm::vec3>()(vertex.vertexPosition) << 1) ^
             (std::hash<glm::vec3>()(vertex.normal) << 2) ^
             (std::hash<glm::vec3>()(vertex.color) << 3) ^
             (std::hash<glm::vec2>()(vertex.textureCoordinates) << 4)));
  }
};

Geometry::Geometry(const std::string& modelName) {
  if (!modelName.empty()) {
    loadModel(modelName, *this);
    transformModel(allVertices, ORIENTATION_ORDER{ROTATE_SCALE_TRANSLATE},
                   glm::vec3(90.0f, 180.0f, 0.0f));
    transformModel(uniqueVertices, ORIENTATION_ORDER{ROTATE_SCALE_TRANSLATE},
                   glm::vec3(90.0f, 180.0f, 0.0f));
  }
}

void Geometry::addVertexPosition(const glm::vec3& position) {
  uniqueVertices.push_back({glm::vec3(0.0f), position});
}

void Geometry::loadModel(const std::string& modelName, Geometry& geometry) {
  std::string baseDir = Lib::path("assets/3D/");
  std::string modelPath = baseDir + modelName + ".obj";

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        modelPath.c_str(), baseDir.c_str())) {
    throw std::runtime_error(warn + err);
  }
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << err << std::endl;
    return;
  }

  std::unordered_map<Geometry::Vertex, uint32_t> tempUniqueVertices;

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Geometry::Vertex vertex{};

      vertex.vertexPosition = {attrib.vertices[3 * index.vertex_index + 0],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};

      vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                       attrib.normals[3 * index.normal_index + 1],
                       attrib.normals[3 * index.normal_index + 2]};

      vertex.textureCoordinates = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (tempUniqueVertices.count(vertex) == 0) {
        tempUniqueVertices[vertex] =
            static_cast<uint32_t>(geometry.uniqueVertices.size());
        geometry.uniqueVertices.push_back(vertex);
      }
      geometry.allVertices.push_back(vertex);
      geometry.indices.push_back(tempUniqueVertices[vertex]);
    }
  }
}

void Geometry::transformModel(std::vector<Vertex>& vertices,
                              ORIENTATION_ORDER order,
                              const glm::vec3& degrees,
                              const glm::vec3& translationDistance,
                              float scale) {
  float angleX = glm::radians(degrees.x);
  float angleY = glm::radians(degrees.y);
  float angleZ = glm::radians(degrees.z);

  glm::mat4 rotationMatrix =
      glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

  for (auto& vertex : vertices) {
    switch (order) {
      case ORIENTATION_ORDER::ROTATE_SCALE_TRANSLATE:
        vertex.vertexPosition =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertexPosition, 1.0f));
        vertex.vertexPosition *= scale;
        vertex.vertexPosition = vertex.vertexPosition + translationDistance;

        vertex.normal =
            glm::vec3(rotationMatrix * glm::vec4(vertex.normal, 1.0f));
        break;
      case ORIENTATION_ORDER::ROTATE_TRANSLATE_SCALE:
        vertex.vertexPosition =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertexPosition, 1.0f));
        vertex.vertexPosition = vertex.vertexPosition + translationDistance;
        vertex.vertexPosition *= scale;

        vertex.normal =
            glm::vec3(rotationMatrix * glm::vec4(vertex.normal, 1.0f));
        break;
    }
  }
  return;
}
