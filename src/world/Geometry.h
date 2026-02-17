#pragma once
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>

#include "vulkan_base/VulkanResources.h"

#include <string>
#include <vector>

enum ORIENTATION_ORDER { CE_ROTATE_SCALE_TRANSLATE = 0, CE_ROTATE_TRANSLATE_SCALE = 1 };

enum GEOMETRY_SHAPE {
  CE_RECTANGLE = 0,
  CE_CUBE = 1,
  CE_SPHERE = 2,
  CE_SPHERE_HR = 3,
  CE_TORUS = 4
};

class Vertex {
public:
  glm::vec3 instance_position{};
  glm::vec3 vertex_position{};
  glm::vec3 normal{};
  glm::vec3 color{};
  glm::vec2 texture_coordinates{};

  static std::vector<VkVertexInputBindingDescription> get_binding_description();
  static std::vector<VkVertexInputAttributeDescription> get_attribute_description();

  bool operator==(const Vertex &other) const {
    return instance_position == other.instance_position &&
           vertex_position == other.vertex_position && normal == other.normal &&
           color == other.color && texture_coordinates == other.texture_coordinates;
  }
};

class Geometry : public Vertex {
public:
  Geometry() = default;
  Geometry(GEOMETRY_SHAPE shape);
  virtual ~Geometry() = default;
  std::vector<Vertex> all_vertices{};
  std::vector<Vertex> unique_vertices{};
  std::vector<uint32_t> indices{};

  CE::Buffer vertex_buffer;
  CE::Buffer index_buffer;

  void add_vertex_position(const glm::vec3 &position);
  static std::vector<uint32_t> create_grid_polygons(const std::vector<uint32_t> &vertices,
                                                    uint32_t grid_width);

protected:
  void create_vertex_buffer(VkCommandBuffer &command_buffer,
                            const VkCommandPool &command_pool,
                          const VkQueue &queue,
                          const std::vector<Vertex> &vertices);
  void create_vertex_buffer(VkCommandBuffer &command_buffer,
                            const VkCommandPool &command_pool,
                            const VkQueue &queue,
                            const std::vector<Vertex> &vertices,
                            CE::Buffer &target_buffer);

  void create_index_buffer(VkCommandBuffer &command_buffer,
                           const VkCommandPool &command_pool,
                         const VkQueue &queue,
                         const std::vector<uint32_t> &indices);
  void create_index_buffer(VkCommandBuffer &command_buffer,
                           const VkCommandPool &command_pool,
                           const VkQueue &queue,
                           const std::vector<uint32_t> &indices,
                           CE::Buffer &target_buffer);

private:
  void load_model(const std::string &model_name, Geometry &geometry);
  void transform_model(std::vector<Vertex> &vertices,
                       ORIENTATION_ORDER order,
                      const glm::vec3 &degrees = glm::vec3(0.0f),
                      const glm::vec3 &translation_distance = glm::vec3(0.0f),
                      float scale = 1.0f);
};

class Shape : public Geometry {
public:
  Shape(GEOMETRY_SHAPE shape,
  bool has_indices,
  VkCommandBuffer &command_buffer,
  const VkCommandPool &command_pool,
        const VkQueue &queue);
};
