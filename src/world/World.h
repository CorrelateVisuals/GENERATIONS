#pragma once

// Simulation/render world aggregate (grid, shapes, camera, uniforms, time).
// Exists to package scene state consumed by compute and graphics passes.
#include "world/Camera.h"
#include "world/Geometry.h"
#include "library/Library.h"
#include "control/Timer.h"
#include "world/RuntimeConfig.h"
#include "vulkan_resources/ShaderInterface.h"

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

	using UniformBufferObject = CE::ShaderInterface::ParameterUBO;

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
	};

	Grid _grid;
	Shape _rectangle;
	Shape _cube;
	Shape _sky_dome;

	UniformBufferObject _ubo;
	Camera _camera;
	Timer _time;
};
