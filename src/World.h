#pragma once
#include "Camera.h"
#include "Timer.h"

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

class World {
 public:
  World(VkCommandBuffer& commandBuffer,
        const VkCommandPool& commandPool,
        const VkQueue& queue);
  ~World();

  struct alignas(16) Cell {
    glm::vec4 instancePosition{};
    glm::vec4 vertexPosition{};
    glm::vec4 normal{};
    glm::vec4 color{};
    glm::ivec4 states{};

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescription();
  };

  struct UniformBufferObject {
    glm::vec4 light;
    glm::ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    alignas(16) glm::mat4 model{1.0f};
    alignas(16) glm::mat4 view{1.0f};
    alignas(16) glm::mat4 projection{1.0f};

    UniformBufferObject(glm::vec4 l, glm::ivec2 xy, float w, float s)
        : light(l), gridXY(xy), waterThreshold(w), cellSize(s){};
  };

  UniformBufferObject _ubo;
  Camera _camera;
  Timer _time;
};
