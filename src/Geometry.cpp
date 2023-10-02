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
                         std::vector<Geometry::Vertex>& vertices,
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
                        modelPath.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Geometry::Vertex, uint32_t> uniqueVertices{};

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Geometry::Vertex vertex{};

      vertex.vertexPosition = {attrib.vertices[3 * index.vertex_index + 0],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};

      vertex.textureCoordinates = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());

        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }
  Geometry::transformModel(vertices, ORIENTATION_ORDER{ROTATE_SCALE_TRANSLATE},
                           rotate, translate, geoSize);
}

void Geometry::loadModel(const std::string& modelPath,
                         std::vector<Geometry::Vertex>& vertices,
    ORIENTATION_ORDER order,
                         const glm::vec3& rotate,
                         const glm::vec3& translate,
                         float geoSize) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warn;
  std::string err;

  tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str(),
                   nullptr);

  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << err << std::endl;
    return;
  }

  for (size_t s = 0; s < shapes.size(); s++) {
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      int fv = 3;
      for (size_t v = 0; v < fv; v++) {
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

        tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
        tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
        tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
        tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
        tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
        tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

        Geometry::Vertex new_vert;
        new_vert.vertexPosition.x = vx;
        new_vert.vertexPosition.y = vy;
        new_vert.vertexPosition.z = vz;

        new_vert.normal.x = nx;
        new_vert.normal.y = ny;
        new_vert.normal.z = nz;

        new_vert.color = new_vert.normal;

        vertices.push_back(new_vert);
      }
      index_offset += fv;
    }
  }
  Geometry::transformModel(vertices, ORIENTATION_ORDER{ ROTATE_SCALE_TRANSLATE },
      rotate, translate, geoSize);
  return;
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
