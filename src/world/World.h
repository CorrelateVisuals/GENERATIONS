#pragma once
#include "Camera.h"
#include "Geometry.h"
#include "io/Library.h"
#include "core/Timer.h"
#include "core/RuntimeConfig.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>
#include <vector>

class World {
public:
  World(VkCommandBuffer &command_buffer,
        const VkCommandPool &command_pool,
        const VkQueue &queue,
        const CE::Runtime::TerrainSettings &terrain_settings);
  ~World();

  struct alignas(16) Cell {
    glm::vec4 instance_position{};
    glm::vec4 vertex_position{};
    glm::vec4 normal{};
    glm::vec4 color{};
    glm::ivec4 states{};

    static std::vector<VkVertexInputBindingDescription> get_binding_description();
    static std::vector<VkVertexInputAttributeDescription> get_attribute_description();
  };

  struct UniformBufferObject {
    glm::vec4 light;
    glm::ivec2 grid_xy;
    float water_threshold;
    float cell_size;
    // x: dead-zone margin, y: shore band width, z: border highlight width, w: reserved
    glm::vec4 water_rules{2.4f, 1.2f, 0.08f, 0.0f};
    alignas(16) ModelViewProjection mvp{};

    UniformBufferObject(glm::vec4 l, glm::ivec2 xy, float w, float s, glm::vec4 rules)
      : light(l), grid_xy(xy), water_threshold(w), cell_size(s), water_rules(rules){};
  };

  struct Grid : public Geometry {
    Vec2UintFast16 size;
    const uint_fast32_t initial_alive_cells;
    const size_t point_count;

    std::vector<uint32_t> point_ids = std::vector<uint32_t>(point_count);
    std::vector<glm::vec3> coordinates = std::vector<glm::vec3>(point_count);
    std::vector<World::Cell> cells = std::vector<World::Cell>(point_count);
    std::vector<Vertex> box_vertices{};
    std::vector<uint32_t> box_indices{};
    CE::Buffer box_vertex_buffer;
    CE::Buffer box_index_buffer;

        Grid(const CE::Runtime::TerrainSettings &terrain_settings,
          VkCommandBuffer &command_buffer,
          const VkCommandPool &command_pool,
          const VkQueue &queue);
    static std::vector<VkVertexInputAttributeDescription> get_attribute_description();

  private:
    std::vector<uint_fast32_t> set_cells_alive_randomly(uint_fast32_t number_of_cells);
  };

  Grid _grid;
  Shape _rectangle;
  Shape _cube;
  Shape _sky_dome;

  UniformBufferObject _ubo;
  Camera _camera;
  Timer _time;
};
