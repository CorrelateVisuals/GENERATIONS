#include "Geometry.h"

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
