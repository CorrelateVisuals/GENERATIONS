#pragma once
#include "Camera.h"
#include "Geometry.h"
#include "io/Library.h"
#include "core/Timer.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>
#include <vector>

class World {
public:
  World(VkCommandBuffer &command_buffer,
        const VkCommandPool &command_pool,
        const VkQueue &queue);
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
    glm::ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    alignas(16) ModelViewProjection mvp{};

    UniformBufferObject(glm::vec4 l, glm::ivec2 xy, float w, float s)
        : light(l), gridXY(xy), waterThreshold(w), cellSize(s){};
  };

  struct Grid : public Geometry {
    vec2_uint_fast16_t size;
    const uint_fast32_t initial_alive_cells;
    const size_t point_count;

    std::vector<uint32_t> point_ids = std::vector<uint32_t>(point_count);
    std::vector<glm::vec3> coordinates = std::vector<glm::vec3>(point_count);
    std::vector<World::Cell> cells = std::vector<World::Cell>(point_count);

    Grid(vec2_uint_fast16_t grid_size,
         uint_fast32_t alive_cells,
         float cell_size,
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

  UniformBufferObject _ubo;
  Camera _camera;
  Timer _time;
};
