#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Geometry.h"
#include "Library.h"

std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> binding{
      {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return binding;
}

std::vector<VkVertexInputAttributeDescription>
Vertex::getAttributeDescription() {
  std::vector<VkVertexInputAttributeDescription> attributes{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Vertex, vertexPosition))},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Vertex, color))},
      {2, 0, VK_FORMAT_R32G32_SFLOAT,
       static_cast<uint32_t>(offsetof(Vertex, textureCoordinates))}};
  return attributes;
}

template <>
struct std::hash<Vertex> {
  size_t operator()(const Vertex& vertex) const {
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

std::vector<uint32_t> Geometry::createGridPolygons(
    const std::vector<uint32_t>& vertices,
    uint32_t gridWidth) {
  std::vector<uint32_t> result;

  uint32_t numRows = static_cast<uint32_t>(vertices.size()) / gridWidth;

  for (uint32_t row = 0; row < numRows - 1; row++) {
    for (uint32_t col = 0; col < gridWidth - 1; col++) {
      // Calculate vertices for the four vertices of the quad
      uint32_t topLeft = row * gridWidth + col;
      uint32_t topRight = topLeft + 1;
      uint32_t bottomLeft = (row + 1) * gridWidth + col;
      uint32_t bottomRight = bottomLeft + 1;

      // Create the first triangle of the quad
      result.push_back(topLeft);
      result.push_back(topRight);
      result.push_back(bottomLeft);

      // Create the second triangle of the quad
      result.push_back(topRight);
      result.push_back(bottomRight);
      result.push_back(bottomLeft);
    }
  }

  return result;
}

void Geometry::createVertexBuffer(VkCommandBuffer& commandBuffer,
                                  const VkCommandPool& commandPool,
                                  const VkQueue& queue,
                                  const auto& vertices) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void* data;
  vkMapMemory(CE::Device::baseDevice->logical, stagingResources.memory, 0,
              bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(CE::Device::baseDevice->logical, stagingResources.memory);

  CE::Buffer::create(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->vertexBuffer);

  CE::Buffer::copy(stagingResources.buffer, this->vertexBuffer.buffer,
                   bufferSize, commandBuffer, commandPool, queue);
}

void Geometry::createIndexBuffer(VkCommandBuffer& commandBuffer,
                                 const VkCommandPool& commandPool,
                                 const VkQueue& queue,
                                 const auto& indices) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void* data;
  vkMapMemory(CE::Device::baseDevice->logical, stagingResources.memory, 0,
              bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(CE::Device::baseDevice->logical, stagingResources.memory);

  CE::Buffer::create(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->indexBuffer);

  CE::Buffer::copy(stagingResources.buffer, this->indexBuffer.buffer,
                   bufferSize, commandBuffer, commandPool, queue);
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
    std::cout << "WARN: " << warn << '\n';
  }
  if (!err.empty()) {
    std::cerr << err << '\n';
    return;
  }

  std::unordered_map<Vertex, uint32_t> tempUniqueVertices;

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};

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
