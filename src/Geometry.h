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
  Geometry(const std::string& modelName = "");
  virtual ~Geometry() = default;
  std::vector<Vertex> allVertices{};
  std::vector<Vertex> uniqueVertices{};
  std::vector<uint32_t> indices{};

  CE::Buffer vertexBuffer{};
  CE::Buffer indexBuffer{};

  void addVertexPosition(const glm::vec3& position);
  static std::vector<uint32_t> createGridPolygons(
      const std::vector<uint32_t>& vertices,
      uint32_t gridWidth);

 protected:
  void createVertexBuffer(VkCommandBuffer& commandBuffer,
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

 private:
  void loadModel(const std::string& modelName, Geometry& geometry);
  void transformModel(std::vector<Vertex>& vertices,
                      ORIENTATION_ORDER order,
                      const glm::vec3& degrees = glm::vec3(0.0f),
                      const glm::vec3& translationDistance = glm::vec3(0.0f),
                      float scale = 1.0f);
};
