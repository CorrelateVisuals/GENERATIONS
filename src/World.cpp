#include "World.h"
#include "CapitalEngine.h"

World::World(VkCommandBuffer& commandBuffer,
             const VkCommandPool& commandPool,
             const VkQueue& queue)
    : _ubo({0.0f, 0.0f, 0.0f, 0.0f}, {0, 0}, 0.0f, 0.0f),
      _camera(0.5f, 1.0f, 45.0f, 0.1f, 100.0f, {0.0f, 0.0f, 1.0f}),
      _time(1.0f) {
  Log::text("{ wWw }", "constructing World");
}

World::~World() {
  Log::text("{ wWw }", "destructing World");
}

std::vector<VkVertexInputBindingDescription>
World::Cell::getBindingDescription() {
  return {};
}

std::vector<VkVertexInputAttributeDescription>
World::Cell::getAttributeDescription() {
  return {};
}
