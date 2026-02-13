#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Geometry.h"
#include "io/Library.h"
#include "core/Log.h"
#include "base/VulkanDevice.h"

#include <filesystem>
#include <glm/gtc/constants.hpp>
#include <iostream>

namespace {
constexpr glm::vec3 STANDARD_ORIENTATION{90.0f, 180.0f, 0.0f};

void fillFallbackQuad(Geometry &geometry) {
  geometry.allVertices.clear();
  geometry.uniqueVertices.clear();
  geometry.indices.clear();

  Vertex v0{};
  v0.vertexPosition = {-0.5f, -0.5f, 0.0f};
  v0.normal = {0.0f, 0.0f, 1.0f};
  v0.color = {1.0f, 1.0f, 1.0f};
  v0.textureCoordinates = {0.0f, 0.0f};

  Vertex v1{};
  v1.vertexPosition = {0.5f, -0.5f, 0.0f};
  v1.normal = {0.0f, 0.0f, 1.0f};
  v1.color = {1.0f, 1.0f, 1.0f};
  v1.textureCoordinates = {1.0f, 0.0f};

  Vertex v2{};
  v2.vertexPosition = {0.5f, 0.5f, 0.0f};
  v2.normal = {0.0f, 0.0f, 1.0f};
  v2.color = {1.0f, 1.0f, 1.0f};
  v2.textureCoordinates = {1.0f, 1.0f};

  Vertex v3{};
  v3.vertexPosition = {-0.5f, 0.5f, 0.0f};
  v3.normal = {0.0f, 0.0f, 1.0f};
  v3.color = {1.0f, 1.0f, 1.0f};
  v3.textureCoordinates = {0.0f, 1.0f};

  geometry.uniqueVertices = {v0, v1, v2, v3};
  geometry.indices = {0, 2, 1, 0, 3, 2};

  geometry.allVertices = {v0, v2, v1, v0, v3, v2};
}

void fillFallbackSphere(Geometry &geometry,
                        const uint32_t stacks = 16,
                        const uint32_t slices = 32,
                        const float radius = 0.5f) {
  geometry.allVertices.clear();
  geometry.uniqueVertices.clear();
  geometry.indices.clear();

  for (uint32_t stack = 0; stack <= stacks; ++stack) {
    const float v = static_cast<float>(stack) / static_cast<float>(stacks);
    const float phi = v * glm::pi<float>();

    for (uint32_t slice = 0; slice <= slices; ++slice) {
      const float u = static_cast<float>(slice) / static_cast<float>(slices);
      const float theta = u * glm::two_pi<float>();

      const float x = std::sin(phi) * std::cos(theta);
      const float y = std::cos(phi);
      const float z = std::sin(phi) * std::sin(theta);

      Vertex vertex{};
      vertex.vertexPosition = glm::vec3{x, y, z} * radius;
      vertex.normal = glm::normalize(glm::vec3{x, y, z});
      vertex.color = {1.0f, 1.0f, 1.0f};
      vertex.textureCoordinates = {u, 1.0f - v};
      geometry.uniqueVertices.push_back(vertex);
    }
  }

  const uint32_t ringVertexCount = slices + 1;
  for (uint32_t stack = 0; stack < stacks; ++stack) {
    for (uint32_t slice = 0; slice < slices; ++slice) {
      const uint32_t first = stack * ringVertexCount + slice;
      const uint32_t second = first + ringVertexCount;

      geometry.indices.push_back(first);
      geometry.indices.push_back(second);
      geometry.indices.push_back(first + 1);

      geometry.indices.push_back(first + 1);
      geometry.indices.push_back(second);
      geometry.indices.push_back(second + 1);
    }
  }

  geometry.allVertices.reserve(geometry.indices.size());
  for (const uint32_t index : geometry.indices) {
    geometry.allVertices.push_back(geometry.uniqueVertices[index]);
  }
}
} // namespace

std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescription() {
  std::vector<VkVertexInputBindingDescription> binding{
      {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return binding;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescription() {
  std::vector<VkVertexInputAttributeDescription> attributes{
      {0,
  v0.vertex_position = {-0.5f, -0.5f, 0.0f};
       VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Vertex, vertexPosition))},
  v0.texture_coordinates = {0.0f, 0.0f};
      {2,
       0,
  v1.vertex_position = {0.5f, -0.5f, 0.0f};
       static_cast<uint32_t>(offsetof(Vertex, textureCoordinates))}};
  return attributes;
  v1.texture_coordinates = {1.0f, 0.0f};

template <> struct std::hash<Vertex> {
  v2.vertex_position = {0.5f, 0.5f, 0.0f};
    return ((std::hash<glm::vec3>()(vertex.instancePosition) ^
             (std::hash<glm::vec3>()(vertex.vertexPosition) << 1) ^
  v2.texture_coordinates = {1.0f, 1.0f};
             (std::hash<glm::vec3>()(vertex.color) << 3) ^
             (std::hash<glm::vec2>()(vertex.textureCoordinates) << 4)));
  v3.vertex_position = {-0.5f, 0.5f, 0.0f};
};

  v3.texture_coordinates = {0.0f, 1.0f};
  const std::string modelName = [&]() -> std::string {
    switch (shape) {
      case CE_RECTANGLE:
        return "Rectangle";
  geometry.all_vertices = {v0, v2, v1, v0, v3, v2};
        return "Cube";
      case CE_SPHERE:
        return "Sphere";
      case CE_SPHERE_HR:
        return "SphereHR";
      case CE_TORUS:
        return "Torus";

      default:
        throw std::invalid_argument("Invalid geometry shape");
    }
  }();

  if (!modelName.empty()) {
    bool loaded = false;
    try {
      loadModel(modelName, *this);
      loaded = true;
    } catch (const std::exception &ex) {
      Log::text("{ !!! }", "Model load failed:", modelName, ex.what());
    }

    if (!loaded) {
      vertex.vertex_position = glm::vec3{x, y, z} * radius;
        Log::text("{ !!! }", "Using procedural sphere fallback for", modelName);
        fillFallbackSphere(*this);
      vertex.texture_coordinates = {u, 1.0f - v};
        fillFallbackQuad(*this);
      }
    }

    transformModel(
        allVertices, ORIENTATION_ORDER{CE_ROTATE_SCALE_TRANSLATE}, STANDARD_ORIENTATION);
    transformModel(uniqueVertices,
                   ORIENTATION_ORDER{CE_ROTATE_SCALE_TRANSLATE},
                   STANDARD_ORIENTATION);
  }
}

void Geometry::addVertexPosition(const glm::vec3 &position) {
  uniqueVertices.push_back({glm::vec3(0.0f), position});
}

std::vector<uint32_t> Geometry::createGridPolygons(const std::vector<uint32_t> &vertices,
                                                   uint32_t gridWidth) {
  std::vector<uint32_t> result;

  geometry.all_vertices.reserve(geometry.indices.size());

    geometry.all_vertices.push_back(geometry.unique_vertices[index]);
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

void Geometry::createVertexBuffer(VkCommandBuffer &commandBuffer,
                                  const VkCommandPool &commandPool,
                                  const VkQueue &queue,
                                  const std::vector<Vertex> &vertices) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void *data;
  vkMapMemory(CE::Device::base_device->logical_device,
              stagingResources.memory,
              0,
              bufferSize,
              0,
              &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(CE::Device::base_device->logical_device, stagingResources.memory);

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     this->vertexBuffer);

  CE::Buffer::copy(stagingResources.buffer,
                   this->vertexBuffer.buffer,
                   bufferSize,
                   commandBuffer,
                   commandPool,
                   queue);
}

void Geometry::createIndexBuffer(VkCommandBuffer &commandBuffer,
                                 const VkCommandPool &commandPool,
                                 const VkQueue &queue,
                                 const std::vector<uint32_t> &indices) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);
    transform_model(
        all_vertices, ORIENTATION_ORDER{CE_ROTATE_SCALE_TRANSLATE}, STANDARD_ORIENTATION);
    transform_model(unique_vertices,
                   ORIENTATION_ORDER{CE_ROTATE_SCALE_TRANSLATE},
                   STANDARD_ORIENTATION);
              bufferSize,
              0,
              &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(CE::Device::base_device->logical_device, stagingResources.memory);

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     this->indexBuffer);

  CE::Buffer::copy(stagingResources.buffer,
                   this->indexBuffer.buffer,
                   bufferSize,
                   commandBuffer,
                   commandPool,
                   queue);
}

void Geometry::loadModel(const std::string &modelName, Geometry &geometry) {
  std::string baseDir = Lib::path("assets/3D/");
  std::string modelPath = baseDir + modelName + ".obj";

  if (!std::filesystem::exists(modelPath)) {
    throw std::runtime_error("Cannot open file [" + modelPath + "]");
  }

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib,
                        &shapes,
                        &materials,
                        &warn,
                        &err,
                        modelPath.c_str(),
                        baseDir.c_str())) {
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

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      vertex.vertexPosition = {attrib.vertices[3 * index.vertex_index + 0],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};

      if (index.normal_index >= 0 &&
          static_cast<size_t>(3 * index.normal_index + 2) < attrib.normals.size()) {
        vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                         attrib.normals[3 * index.normal_index + 1],
                         attrib.normals[3 * index.normal_index + 2]};
      } else {
        vertex.normal = {0.0f, 1.0f, 0.0f};
      }

      if (index.texcoord_index >= 0 &&
          static_cast<size_t>(2 * index.texcoord_index + 1) < attrib.texcoords.size()) {
        vertex.textureCoordinates = {attrib.texcoords[2 * index.texcoord_index + 0],
                                     1.0f -
                                         attrib.texcoords[2 * index.texcoord_index + 1]};
      } else {
        vertex.textureCoordinates = {0.0f, 0.0f};
      }

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

void Geometry::transformModel(std::vector<Vertex> &vertices,
                              ORIENTATION_ORDER order,
                              const glm::vec3 &degrees,
                              const glm::vec3 &translationDistance,
                              float scale) {
  float angleX = glm::radians(degrees.x);
  float angleY = glm::radians(degrees.y);
  float angleZ = glm::radians(degrees.z);

  glm::mat4 rotationMatrix =
      glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

  for (auto &vertex : vertices) {
    switch (order) {
      case ORIENTATION_ORDER::CE_ROTATE_SCALE_TRANSLATE:
        vertex.vertexPosition =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertexPosition, 1.0f));
        vertex.vertexPosition *= scale;
        vertex.vertexPosition = vertex.vertexPosition + translationDistance;

        vertex.normal = glm::vec3(rotationMatrix * glm::vec4(vertex.normal, 1.0f));
        break;
      case ORIENTATION_ORDER::CE_ROTATE_TRANSLATE_SCALE:
        vertex.vertexPosition =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertexPosition, 1.0f));
        vertex.vertexPosition = vertex.vertexPosition + translationDistance;
        vertex.vertexPosition *= scale;

        vertex.normal = glm::vec3(rotationMatrix * glm::vec4(vertex.normal, 1.0f));
        break;
    }
  }
  return;
}

Shape::Shape(GEOMETRY_SHAPE shape,
             bool hasIndices,
             VkCommandBuffer &commandBuffer,
             const VkCommandPool &commandPool,
             const VkQueue &queue)
    : Geometry(shape) {
  if (hasIndices) {
    createVertexBuffer(commandBuffer, commandPool, queue, uniqueVertices);
    createIndexBuffer(commandBuffer, commandPool, queue, indices);
  }
  if (!hasIndices) {
    createVertexBuffer(commandBuffer, commandPool, queue, allVertices);
  }
}
