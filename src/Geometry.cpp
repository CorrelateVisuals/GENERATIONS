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

void Geometry::loadModel(const std::string& modelPath,
                         std::vector<Geometry::Vertex>& allVertices,
                         std::vector<Geometry::Vertex>& uniqueVertices,
                         std::vector<uint32_t>& indices,
                         ORIENTATION_ORDER order,
                         const glm::vec3& rotate,
                         const glm::vec3& translate,
                         float geoSize) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        modelPath.c_str(), nullptr)) {
    throw std::runtime_error(warn + err);
  }
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << err << std::endl;
    return;
  }

  std::unordered_map<Geometry::Vertex, uint32_t> tempUniqueVertices{};
  int idx = 0;
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
            static_cast<uint32_t>(uniqueVertices.size());
        uniqueVertices.push_back(vertex);
      }
      allVertices.push_back(vertex);
      indices.push_back(tempUniqueVertices[vertex]);
    }
  }
  transformModel(uniqueVertices, order, rotate, translate, geoSize);
  transformModel(allVertices, order, rotate, translate, geoSize);
}

void Geometry::transformModel(auto& vertices,
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
        break;
      case ORIENTATION_ORDER::ROTATE_TRANSLATE_SCALE:
        vertex.vertexPosition =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertexPosition, 1.0f));
        vertex.vertexPosition = vertex.vertexPosition + translationDistance;
        vertex.vertexPosition *= scale;
        break;
    }
  }
  return;
}
