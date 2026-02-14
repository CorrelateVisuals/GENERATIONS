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
  geometry.all_vertices.clear();
  geometry.unique_vertices.clear();
  geometry.indices.clear();

  Vertex v0{};
  v0.vertex_position = {-0.5f, -0.5f, 0.0f};
  v0.normal = {0.0f, 0.0f, 1.0f};
  v0.color = {1.0f, 1.0f, 1.0f};
  v0.texture_coordinates = {0.0f, 0.0f};

  Vertex v1{};
  v1.vertex_position = {0.5f, -0.5f, 0.0f};
  v1.normal = {0.0f, 0.0f, 1.0f};
  v1.color = {1.0f, 1.0f, 1.0f};
  v1.texture_coordinates = {1.0f, 0.0f};

  Vertex v2{};
  v2.vertex_position = {0.5f, 0.5f, 0.0f};
  v2.normal = {0.0f, 0.0f, 1.0f};
  v2.color = {1.0f, 1.0f, 1.0f};
  v2.texture_coordinates = {1.0f, 1.0f};

  Vertex v3{};
  v3.vertex_position = {-0.5f, 0.5f, 0.0f};
  v3.normal = {0.0f, 0.0f, 1.0f};
  v3.color = {1.0f, 1.0f, 1.0f};
  v3.texture_coordinates = {0.0f, 1.0f};

  geometry.unique_vertices = {v0, v1, v2, v3};
  geometry.indices = {0, 2, 1, 0, 3, 2};

  geometry.all_vertices = {v0, v2, v1, v0, v3, v2};
}

void fillFallbackSphere(Geometry &geometry,
                        const uint32_t stacks = 16,
                        const uint32_t slices = 32,
                        const float radius = 0.5f) {
  geometry.all_vertices.clear();
  geometry.unique_vertices.clear();
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
      vertex.vertex_position = glm::vec3{x, y, z} * radius;
      vertex.normal = glm::normalize(glm::vec3{x, y, z});
      vertex.color = {1.0f, 1.0f, 1.0f};
      vertex.texture_coordinates = {u, 1.0f - v};
      geometry.unique_vertices.push_back(vertex);
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

  geometry.all_vertices.reserve(geometry.indices.size());
  for (const uint32_t index : geometry.indices) {
    geometry.all_vertices.push_back(geometry.unique_vertices[index]);
  }
}
} // namespace

std::vector<VkVertexInputBindingDescription> Vertex::get_binding_description() {
  std::vector<VkVertexInputBindingDescription> binding{
      {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return binding;
}

std::vector<VkVertexInputAttributeDescription> Vertex::get_attribute_description() {
  std::vector<VkVertexInputAttributeDescription> attributes{
      {0,
       0,
       VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Vertex, vertex_position))},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, color))},
      {2,
       0,
       VK_FORMAT_R32G32_SFLOAT,
       static_cast<uint32_t>(offsetof(Vertex, texture_coordinates))}};
  return attributes;
}

template <> struct std::hash<Vertex> {
  size_t operator()(const Vertex &vertex) const {
    return ((std::hash<glm::vec3>()(vertex.instance_position) ^
             (std::hash<glm::vec3>()(vertex.vertex_position) << 1) ^
             (std::hash<glm::vec3>()(vertex.normal) << 2) ^
             (std::hash<glm::vec3>()(vertex.color) << 3) ^
             (std::hash<glm::vec2>()(vertex.texture_coordinates) << 4)));
  }
};

Geometry::Geometry(GEOMETRY_SHAPE shape) {
  const std::string model_name = [&]() -> std::string {
    switch (shape) {
      case CE_RECTANGLE:
        return "Rectangle";
      case CE_CUBE:
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

  if (!model_name.empty()) {
    if (Log::gpu_trace_enabled()) {
      Log::text("{ mdl }",
                "Selected model",
                model_name,
                "shape",
                static_cast<uint32_t>(shape));
    }

    bool loaded = false;
    try {
      load_model(model_name, *this);
      loaded = true;
    } catch (const std::exception &ex) {
      Log::text("{ !!! }", "Model load failed:", model_name, ex.what());
    }

    if (!loaded) {
      if (shape == CE_SPHERE || shape == CE_SPHERE_HR) {
        Log::text("{ !!! }", "Using procedural sphere fallback for", model_name);
        fillFallbackSphere(*this);
      } else {
        fillFallbackQuad(*this);
      }
    }

    transform_model(
        all_vertices, ORIENTATION_ORDER{CE_ROTATE_SCALE_TRANSLATE}, STANDARD_ORIENTATION);
    transform_model(unique_vertices,
                    ORIENTATION_ORDER{CE_ROTATE_SCALE_TRANSLATE},
                    STANDARD_ORIENTATION);
  }
}

void Geometry::add_vertex_position(const glm::vec3 &position) {
  unique_vertices.push_back({glm::vec3(0.0f), position});
}

std::vector<uint32_t> Geometry::create_grid_polygons(const std::vector<uint32_t> &vertices,
                                                     uint32_t grid_width) {
  std::vector<uint32_t> result;

  uint32_t numRows = static_cast<uint32_t>(vertices.size()) / grid_width;

  for (uint32_t row = 0; row < numRows - 1; row++) {
    for (uint32_t col = 0; col < grid_width - 1; col++) {
      // Calculate vertices for the four vertices of the quad
      uint32_t topLeft = row * grid_width + col;
      uint32_t topRight = topLeft + 1;
      uint32_t bottomLeft = (row + 1) * grid_width + col;
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

void Geometry::create_vertex_buffer(VkCommandBuffer &command_buffer,
                                    const VkCommandPool &command_pool,
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
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Map staging vertex memory", stagingResources.memory, bufferSize);
  }
  vkMapMemory(CE::Device::base_device->logical_device,
              stagingResources.memory,
              0,
              bufferSize,
              0,
              &data);
  if (Log::gpu_trace_enabled()) {
    Log::text("{ WR }", "Write host->staging vertex bytes", bufferSize);
  }
  memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Unmap staging vertex memory", stagingResources.memory);
  }
  vkUnmapMemory(CE::Device::base_device->logical_device, stagingResources.memory);

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     this->vertex_buffer);

  CE::Buffer::copy(stagingResources.buffer,
                   this->vertex_buffer.buffer,
                   bufferSize,
                   command_buffer,
                   command_pool,
                   queue);
}

void Geometry::create_index_buffer(VkCommandBuffer &command_buffer,
                                   const VkCommandPool &command_pool,
                                   const VkQueue &queue,
                                   const std::vector<uint32_t> &index_data) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(index_data[0]) * index_data.size();

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void *data;
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Map staging index memory", stagingResources.memory, bufferSize);
  }
  vkMapMemory(CE::Device::base_device->logical_device,
              stagingResources.memory,
              0,
              bufferSize,
              0,
              &data);
  if (Log::gpu_trace_enabled()) {
    Log::text("{ WR }", "Write host->staging index bytes", bufferSize);
  }
  memcpy(data, index_data.data(), static_cast<size_t>(bufferSize));
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Unmap staging index memory", stagingResources.memory);
  }
  vkUnmapMemory(CE::Device::base_device->logical_device, stagingResources.memory);

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     this->index_buffer);

  CE::Buffer::copy(stagingResources.buffer,
                   this->index_buffer.buffer,
                   bufferSize,
                   command_buffer,
                   command_pool,
                   queue);
}

void Geometry::load_model(const std::string &model_name, Geometry &geometry) {
  std::string baseDir = Lib::path("assets/3D/");
  std::string modelPath = baseDir + model_name + ".obj";

  if (Log::gpu_trace_enabled()) {
    Log::text("{ mdl }", "Load model path", modelPath);
  }

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

  if (Log::gpu_trace_enabled()) {
    Log::text("{ mdl }",
              "Loaded model",
              model_name,
              "shapes",
              shapes.size(),
              "materials",
              materials.size(),
              "positions",
              attrib.vertices.size() / 3,
              "normals",
              attrib.normals.size() / 3,
              "uvs",
              attrib.texcoords.size() / 2);
  }

  std::unordered_map<Vertex, uint32_t> tempUniqueVertices;

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      vertex.vertex_position = {attrib.vertices[3 * index.vertex_index + 0],
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
        vertex.texture_coordinates = {attrib.texcoords[2 * index.texcoord_index + 0],
                                      1.0f -
                                          attrib.texcoords[2 * index.texcoord_index + 1]};
      } else {
        vertex.texture_coordinates = {0.0f, 0.0f};
      }

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (!tempUniqueVertices.contains(vertex)) {
        tempUniqueVertices[vertex] =
            static_cast<uint32_t>(geometry.unique_vertices.size());
        geometry.unique_vertices.push_back(vertex);
      }
      geometry.all_vertices.push_back(vertex);
      geometry.indices.push_back(tempUniqueVertices[vertex]);
    }
  }
}

void Geometry::transform_model(std::vector<Vertex> &vertices,
                               ORIENTATION_ORDER order,
                               const glm::vec3 &degrees,
                               const glm::vec3 &translation_distance,
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
        vertex.vertex_position =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertex_position, 1.0f));
        vertex.vertex_position *= scale;
        vertex.vertex_position = vertex.vertex_position + translation_distance;

        vertex.normal = glm::vec3(rotationMatrix * glm::vec4(vertex.normal, 1.0f));
        break;
      case ORIENTATION_ORDER::CE_ROTATE_TRANSLATE_SCALE:
        vertex.vertex_position =
            glm::vec3(rotationMatrix * glm::vec4(vertex.vertex_position, 1.0f));
        vertex.vertex_position = vertex.vertex_position + translation_distance;
        vertex.vertex_position *= scale;

        vertex.normal = glm::vec3(rotationMatrix * glm::vec4(vertex.normal, 1.0f));
        break;
    }
  }
  return;
}

Shape::Shape(GEOMETRY_SHAPE shape,
             bool has_indices,
             VkCommandBuffer &command_buffer,
             const VkCommandPool &command_pool,
             const VkQueue &queue)
    : Geometry(shape) {
  if (has_indices) {
    create_vertex_buffer(command_buffer, command_pool, queue, unique_vertices);
    create_index_buffer(command_buffer, command_pool, queue, indices);
  }
  if (!has_indices) {
    create_vertex_buffer(command_buffer, command_pool, queue, all_vertices);
  }
}
