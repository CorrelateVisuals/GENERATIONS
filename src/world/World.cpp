#include "World.h"
#include "Geometry.h"
#include "Terrain.h"
#include "core/Log.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>

namespace {
constexpr glm::ivec2 GRID_SIZE = {50, 50};
constexpr uint_fast32_t NUMBER_OF_ALIVE_CELLS = 200;
constexpr float CELL_SIZE = 0.5f;

constexpr float TIMER_SPEED = 25.0f;
constexpr float WATER_THRESHOLD = 0.1f;
constexpr glm::vec4 LIGHT_POS = {0.0f, 20.0f, 20.0f, 0.0f};

constexpr float ZOOM_SPEED = 0.5f;
constexpr float PANNING_SPEED = 1.2f;
constexpr float FIELD_OF_VIEW = 40.0f;
constexpr float NEAR_CLIPPING = 0.1f;
constexpr float FAR_CLIPPING = 1000.0f;
constexpr glm::vec3 CAMERA_POSITION = {0.0f, 0.0f, 60.0f};
constexpr float ARCBALL_TUMBLE_MULT = 0.9f;
constexpr float ARCBALL_PAN_MULT = 0.85f;
constexpr float ARCBALL_DOLLY_MULT = 0.8f;

constexpr GEOMETRY_SHAPE cube = CE_CUBE;
constexpr GEOMETRY_SHAPE rectangle = CE_RECTANGLE;
constexpr GEOMETRY_SHAPE sphere = CE_SPHERE;
} // namespace

World::World(VkCommandBuffer &command_buffer,
       const VkCommandPool &command_pool,
             const VkQueue &queue)
    : _grid(
      GRID_SIZE, NUMBER_OF_ALIVE_CELLS, CELL_SIZE, command_buffer, command_pool, queue),
    _rectangle(rectangle, true, command_buffer, command_pool, queue),
    _cube(sphere, false, command_buffer, command_pool, queue),
      _ubo(LIGHT_POS, GRID_SIZE, WATER_THRESHOLD, CELL_SIZE), _camera(ZOOM_SPEED,
                                                                      PANNING_SPEED,
                                                                      FIELD_OF_VIEW,
                                                                      NEAR_CLIPPING,
                                                                      FAR_CLIPPING,
                                                                      CAMERA_POSITION),
      _time(TIMER_SPEED) {
  const float halfGridX = 0.5f * static_cast<float>(GRID_SIZE.x) * CELL_SIZE;
  const float halfGridY = 0.5f * static_cast<float>(GRID_SIZE.y) * CELL_SIZE;
  const float sceneRadius = std::sqrt(halfGridX * halfGridX + halfGridY * halfGridY);
    _camera.configure_arcball(glm::vec3(0.0f, 0.0f, 0.0f), sceneRadius);
    _camera.configure_arcball_multipliers(
      ARCBALL_TUMBLE_MULT, ARCBALL_PAN_MULT, ARCBALL_DOLLY_MULT);

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

World::Grid::Grid(Vec2UintFast16 grid_size,
          uint_fast32_t alive_cells,
          float cell_size,
          VkCommandBuffer &command_buffer,
          const VkCommandPool &command_pool,
                  const VkQueue &queue)
  : size(grid_size), initial_alive_cells(alive_cells), point_count(size.x * size.y) {
  Terrain::Config terrainLayer1 = {.dimensions = size,
                                   .roughness = 0.4f,
                                   .octaves = 10,
                                   .scale = 1.1f,
                                   .amplitude = 10.0f,
                                   .exponent = 2.0f,
                                   .frequency = 2.0f,
                                     .height_offset = 0.0f};
  Terrain terrain(terrainLayer1);

  Terrain::Config terrainLayer2 = {.dimensions = size,
                                   .roughness = 1.0f,
                                   .octaves = 10,
                                   .scale = 1.1f,
                                   .amplitude = 1.0f,
                                   .exponent = 1.0f,
                                   .frequency = 2.0f,
                                     .height_offset = 0.0f};
  Terrain terrainSurface(terrainLayer2);

  std::vector<float> terrainPerlinGrid1 = terrain.generate_perlin_grid();
  std::vector<float> terrainPerlinGrid2 = terrainSurface.generate_perlin_grid();
  const float blendFactor = 0.5f;

  std::vector<bool> is_alive_indices(point_count, false);
  std::vector<uint_fast32_t> alive_cell_indices = set_cells_alive_randomly(initial_alive_cells);
  for (int alive_index : alive_cell_indices) {
    is_alive_indices[alive_index] = true;
  }

  const glm::vec4 red{1.0f, 0.0f, 0.0f, 1.0f};
  const glm::vec4 blue{0.0f, 0.0f, 1.0f, 1.0f};
  const glm::ivec4 alive{1, 0, 0, 0};
  const glm::ivec4 dead{-1, 0, 0, 0};

  const float startX = (size.x - 1) / -2.0f;
  const float startY = (size.y - 1) / -2.0f;
  for (uint_fast32_t i = 0; i < point_count; ++i) {
    point_ids[i] = i;

    float height = terrain.linear_interpolation_function(
        terrainPerlinGrid1[i], terrainPerlinGrid2[i], blendFactor);
    coordinates[i] = {(startX + i % size.x), (startY + i / size.x), height};
    add_vertex_position(coordinates[i]);

    const bool is_alive = is_alive_indices[i];

    cells[i].instance_position = {coordinates[i], is_alive ? cell_size : 0.0f};
    cells[i].color = is_alive ? blue : red;
    cells[i].states = is_alive ? alive : dead;
  }
  indices = create_grid_polygons(point_ids, static_cast<int>(size.x));
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
