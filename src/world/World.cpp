#include "World.h"
#include "Geometry.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {
GEOMETRY_SHAPE resolve_shape(const int value, const GEOMETRY_SHAPE fallback) {
  switch (value) {
  case CE_RECTANGLE:
    return CE_RECTANGLE;
  case CE_CUBE:
    return CE_CUBE;
  case CE_SPHERE:
    return CE_SPHERE;
  case CE_SPHERE_HR:
    return CE_SPHERE_HR;
  case CE_TORUS:
    return CE_TORUS;
  default:
    return fallback;
  }
}
} // namespace

World::World(VkCommandBuffer &command_buffer,
       const VkCommandPool &command_pool,
       const VkQueue &queue,
       const CE::Runtime::TerrainSettings &terrain_settings)
    : _grid(terrain_settings,
            command_buffer, command_pool, queue),
      _rectangle(resolve_shape(CE::Runtime::get_world_settings().rectangle_shape,
             CE_RECTANGLE),
     true,
     command_buffer,
     command_pool,
     queue),
      _cube(resolve_shape(CE::Runtime::get_world_settings().sphere_shape, CE_SPHERE),
      false,
      command_buffer,
      command_pool,
      queue),
      _sky_dome(CE_SPHERE_HR,
            false,
            command_buffer,
            command_pool,
            queue),
      _ubo(glm::vec4(CE::Runtime::get_world_settings().light_pos[0],
         CE::Runtime::get_world_settings().light_pos[1],
         CE::Runtime::get_world_settings().light_pos[2],
         CE::Runtime::get_world_settings().light_pos[3]),
           glm::ivec2(terrain_settings.grid_width, terrain_settings.grid_height),
     CE::Runtime::get_world_settings().water_threshold,
           terrain_settings.cell_size,
           glm::vec4(CE::Runtime::get_world_settings().water_dead_zone_margin,
             CE::Runtime::get_world_settings().water_shore_band_width,
             CE::Runtime::get_world_settings().water_border_highlight_width,
             terrain_settings.absolute_height)),
      _camera(CE::Runtime::get_world_settings().zoom_speed,
        CE::Runtime::get_world_settings().panning_speed,
        CE::Runtime::get_world_settings().field_of_view,
        CE::Runtime::get_world_settings().near_clipping,
        CE::Runtime::get_world_settings().far_clipping,
        glm::vec3(CE::Runtime::get_world_settings().camera_position[0],
      CE::Runtime::get_world_settings().camera_position[1],
      CE::Runtime::get_world_settings().camera_position[2])),
      _time(CE::Runtime::get_world_settings().timer_speed) {
  const float halfGridX = 0.5f * static_cast<float>(terrain_settings.grid_width) * terrain_settings.cell_size;
  const float halfGridY = 0.5f * static_cast<float>(terrain_settings.grid_height) * terrain_settings.cell_size;
  const float sceneRadius = std::sqrt(halfGridX * halfGridX + halfGridY * halfGridY);
    _camera.configure_arcball(glm::vec3(0.0f, 0.0f, 0.0f), sceneRadius);
    _camera.configure_arcball_multipliers(
      CE::Runtime::get_world_settings().arcball_tumble_mult,
      CE::Runtime::get_world_settings().arcball_pan_mult,
      CE::Runtime::get_world_settings().arcball_dolly_mult);
    _camera.set_preset_view(4);

  Log::text("{ wWw }", "constructing World");
}

World::~World() {
  Log::text("{ wWw }", "destructing World");
}

std::vector<VkVertexInputBindingDescription> World::Cell::get_binding_description() {
  std::vector<VkVertexInputBindingDescription> description{
      {0, sizeof(Cell), VK_VERTEX_INPUT_RATE_INSTANCE},
      {1, sizeof(Shape::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
  return description;
}

std::vector<VkVertexInputAttributeDescription> World::Cell::get_attribute_description() {
  std::vector<VkVertexInputAttributeDescription> description{
      {0,
       0,
       VK_FORMAT_R32G32B32A32_SFLOAT,
      static_cast<uint32_t>(offsetof(Cell, instance_position))},
      {1,
       1,
       VK_FORMAT_R32G32B32A32_SFLOAT,
        static_cast<uint32_t>(offsetof(Shape::Vertex, vertex_position))},
      {2,
       1,
       VK_FORMAT_R32G32B32A32_SFLOAT,
       static_cast<uint32_t>(offsetof(Shape::Vertex, normal))},
      {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(Cell, color))},
      {4, 0, VK_FORMAT_R32G32B32A32_SINT, static_cast<uint32_t>(offsetof(Cell, states))}};
  return description;
};

World::Grid::Grid(const CE::Runtime::TerrainSettings &terrain_settings,
          VkCommandBuffer &command_buffer,
          const VkCommandPool &command_pool,
                  const VkQueue &queue)
  : size(glm::ivec2(terrain_settings.grid_width, terrain_settings.grid_height)),
    initial_alive_cells(terrain_settings.alive_cells),
    point_count(size.x * size.y) {
  std::vector<bool> is_alive_indices(point_count, false);
  std::vector<uint_fast32_t> alive_cell_indices = set_cells_alive_randomly(initial_alive_cells);
  for (int alive_index : alive_cell_indices) {
    is_alive_indices[alive_index] = true;
  }

  const glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
  const glm::vec4 grey{0.5f, 0.5f, 0.5f, 1.0f};
  const glm::ivec4 alive{1, -1, 0, -1};
  const glm::ivec4 dead{-1, -1, 0, -1};

  const float startX = (size.x - 1) / -2.0f;
  const float startY = (size.y - 1) / -2.0f;
  const float absoluteHeight = terrain_settings.absolute_height;
  const float box_depth =
      std::max(terrain_settings.cell_size * 18.0f,
               std::max(static_cast<float>(size.x), static_cast<float>(size.y)) *
                   terrain_settings.cell_size * 0.35f);
  for (uint_fast32_t i = 0; i < point_count; ++i) {
    point_ids[i] = i;

    coordinates[i] = {(startX + i % size.x), (startY + i / size.x), absoluteHeight};
    add_vertex_position(coordinates[i]);

    const bool is_alive = is_alive_indices[i];

    cells[i].instance_position = {(startX + i % size.x),
                    (startY + i / size.x),
                    absoluteHeight,
                    is_alive ? terrain_settings.cell_size * 1.6f : 0.0f};
    cells[i].color = is_alive ? white : grey;
    cells[i].states = is_alive ? alive : dead;
  }

  const uint32_t grid_width = static_cast<uint32_t>(size.x);
  const uint32_t grid_height = static_cast<uint32_t>(size.y);

  indices = create_grid_polygons(point_ids, static_cast<int>(grid_width));

  const uint32_t bottom_base = static_cast<uint32_t>(unique_vertices.size());
  unique_vertices.reserve(unique_vertices.size() + point_count);
  for (uint_fast32_t i = 0; i < point_count; ++i) {
    const glm::vec3 bottom_position{
        coordinates[i].x,
        coordinates[i].y,
        coordinates[i].z - box_depth,
    };
    add_vertex_position(bottom_position);
  }

  // Bottom face (opposite winding of top face)
  for (uint32_t row = 0; row < grid_height - 1; ++row) {
    for (uint32_t col = 0; col < grid_width - 1; ++col) {
      const uint32_t top_left = row * grid_width + col;
      const uint32_t top_right = top_left + 1;
      const uint32_t bottom_left = (row + 1) * grid_width + col;
      const uint32_t bottom_right = bottom_left + 1;

      indices.push_back(bottom_base + top_left);
      indices.push_back(bottom_base + bottom_left);
      indices.push_back(bottom_base + top_right);

      indices.push_back(bottom_base + top_right);
      indices.push_back(bottom_base + bottom_left);
      indices.push_back(bottom_base + bottom_right);
    }
  }

  // North wall (y min, outward -Y)
  for (uint32_t col = 0; col < grid_width - 1; ++col) {
    const uint32_t t0 = col;
    const uint32_t t1 = col + 1;
    const uint32_t b0 = bottom_base + t0;
    const uint32_t b1 = bottom_base + t1;

    indices.push_back(t0);
    indices.push_back(b0);
    indices.push_back(t1);

    indices.push_back(t1);
    indices.push_back(b0);
    indices.push_back(b1);
  }

  // South wall (y max, outward +Y)
  const uint32_t south_row = grid_height - 1;
  for (uint32_t col = 0; col < grid_width - 1; ++col) {
    const uint32_t t0 = south_row * grid_width + col;
    const uint32_t t1 = t0 + 1;
    const uint32_t b0 = bottom_base + t0;
    const uint32_t b1 = bottom_base + t1;

    indices.push_back(t0);
    indices.push_back(t1);
    indices.push_back(b0);

    indices.push_back(t1);
    indices.push_back(b1);
    indices.push_back(b0);
  }

  // West wall (x min, outward -X)
  for (uint32_t row = 0; row < grid_height - 1; ++row) {
    const uint32_t t0 = row * grid_width;
    const uint32_t t1 = (row + 1) * grid_width;
    const uint32_t b0 = bottom_base + t0;
    const uint32_t b1 = bottom_base + t1;

    indices.push_back(t0);
    indices.push_back(t1);
    indices.push_back(b0);

    indices.push_back(t1);
    indices.push_back(b1);
    indices.push_back(b0);
  }

  // East wall (x max, outward +X)
  const uint32_t east_col = grid_width - 1;
  for (uint32_t row = 0; row < grid_height - 1; ++row) {
    const uint32_t t0 = row * grid_width + east_col;
    const uint32_t t1 = (row + 1) * grid_width + east_col;
    const uint32_t b0 = bottom_base + t0;
    const uint32_t b1 = bottom_base + t1;

    indices.push_back(t0);
    indices.push_back(b0);
    indices.push_back(t1);

    indices.push_back(t1);
    indices.push_back(b0);
    indices.push_back(b1);
  }

  create_vertex_buffer(command_buffer, command_pool, queue, unique_vertices);
  create_index_buffer(command_buffer, command_pool, queue, indices);
}

std::vector<VkVertexInputAttributeDescription> World::Grid::get_attribute_description() {
  std::vector<VkVertexInputAttributeDescription> attributes{
      {0,
       0,
       VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(Grid::Vertex, vertex_position))}};
  return attributes;
}

std::vector<uint_fast32_t>
World::Grid::set_cells_alive_randomly(uint_fast32_t number_of_cells) {
  const uint_fast32_t targetCount =
      std::min<uint_fast32_t>(number_of_cells, static_cast<uint_fast32_t>(point_count));

  std::vector<uint_fast32_t> cell_ids(point_count);
  std::iota(cell_ids.begin(), cell_ids.end(), 0);

  std::random_device random;
  std::mt19937 generate(random());
  std::shuffle(cell_ids.begin(), cell_ids.end(), generate);

  cell_ids.resize(targetCount);
  std::sort(cell_ids.begin(), cell_ids.end());
  return cell_ids;
}
