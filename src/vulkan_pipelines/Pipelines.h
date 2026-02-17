#pragma once

// Runtime pipeline graph assembly for configured render/compute passes.
// Exists to bridge scene/runtime config into concrete Vulkan pipeline objects.
#include <glm/glm.hpp>

#include "library/Library.h"
#include "control/Window.h"
#include "world/World.h"
#include "vulkan_base/VulkanPipeline.h"
#include "world/RuntimeConfig.h"

class VulkanMechanics;
class VulkanResources;

class Pipelines {
public:
	Pipelines(VulkanMechanics &mechanics, VulkanResources &resources);
	Pipelines(const Pipelines &) = delete;
	Pipelines &operator=(const Pipelines &) = delete;
	Pipelines(Pipelines &&) = delete;
	Pipelines &operator=(Pipelines &&) = delete;
	~Pipelines();

	struct ComputeLayout : public CE::PipelineLayout {
		ComputeLayout(CE::DescriptorInterface &interface, CE::PushConstants &push_constant) {
			create_layout(interface.set_layout, push_constant);
		}
	};

	struct GraphicsLayout : public CE::PipelineLayout {
		GraphicsLayout(CE::DescriptorInterface &interface) {
			create_layout(interface.set_layout);
		}
	};

	struct Render : public CE::RenderPass {
		Render(CE::Swapchain &swapchain,
					 const CE::Image &msaa_image,
					 const VkImageView &depth_view) {
			create(msaa_image.info.samples, swapchain.image_format);
			create_framebuffers(swapchain, msaa_image.view, depth_view);
		}
	};

	struct Configuration : public CE::PipelinesConfiguration {
		static constexpr uint32_t ceil_div(const uint32_t value, const uint32_t divisor) {
			return (value + divisor - 1) / divisor;
		}

		static std::array<uint32_t, 3>
		default_work_groups(const std::string &pipeline_name,
												const Vec2UintFast16 grid_size,
												const VkExtent2D &swapchain_extent) {
			const auto compute_groups_2d = [&](const uint32_t tile_x,
																	const uint32_t tile_y) -> std::array<uint32_t, 3> {
				return {ceil_div(static_cast<uint32_t>(grid_size.x), tile_x),
								ceil_div(static_cast<uint32_t>(grid_size.y), tile_y),
								1};
			};

			if (pipeline_name.rfind("Compute", 0) == 0) {
				return compute_groups_2d(16, 16);
			}
			if (pipeline_name == "Engine") {
				return compute_groups_2d(16, 16);
			}
			if (pipeline_name == "SeedCells") {
				return compute_groups_2d(16, 16);
			}
			if (pipeline_name == "PostFX") {
				return {ceil_div(swapchain_extent.width, 16),
					ceil_div(swapchain_extent.height, 16),
								1};
			}
			return {1, 1, 1};
		}

		static CE::PipelinesConfiguration::Graphics
		make_graphics(const CE::Runtime::DrawOpId draw_op,
									const std::vector<std::string> &shaders) {
			if (draw_op == CE::Runtime::DrawOpId::InstancedCells) {
				return Graphics{.shaders = shaders,
												.vertex_attributes = World::Cell::get_attribute_description(),
												.vertex_bindings = World::Cell::get_binding_description()};
			}

			if (draw_op == CE::Runtime::DrawOpId::IndexedGrid ||
					draw_op == CE::Runtime::DrawOpId::IndexedGridBox) {
				return Graphics{.shaders = shaders,
												.vertex_attributes = World::Grid::get_attribute_description(),
												.vertex_bindings = World::Grid::get_binding_description()};
			}
			return Graphics{.shaders = shaders,
											.vertex_attributes = Shape::get_attribute_description(),
											.vertex_bindings = Shape::get_binding_description()};
		}

		Configuration(VkRenderPass &render_pass,
						const VkPipelineLayout &graphics_layout,
						const VkPipelineLayout &compute_layout,
						VkSampleCountFlagBits &msaa_samples,
						const Vec2UintFast16 grid_size,
						const VkExtent2D &swapchain_extent) {
			const auto &runtime_definitions = CE::Runtime::get_pipeline_definitions();
			if (!runtime_definitions.empty()) {
				for (const auto &[pipeline_name, definition] : runtime_definitions) {
					if (definition.is_compute) {
						std::array<uint32_t, 3> work_groups = definition.work_groups;
						if (work_groups[0] == 0 || work_groups[1] == 0 || work_groups[2] == 0) {
							work_groups = default_work_groups(pipeline_name, grid_size, swapchain_extent);
						}
						pipeline_map.emplace(pipeline_name,
																 Compute{.shaders = definition.shaders,
																				 .work_groups = work_groups});
						continue;
					}

					CE::Runtime::DrawOpId draw_selector = CE::Runtime::get_graphics_draw_op_id(pipeline_name);
					if (draw_selector == CE::Runtime::DrawOpId::Unknown) {
						draw_selector = CE::Runtime::DrawOpId::IndexedRectangle;
					}
					pipeline_map.emplace(pipeline_name,
															 make_graphics(draw_selector, definition.shaders));
				}
			} else {
				pipeline_map.emplace(
						"Engine",
						Compute{.shaders = {"Comp"},
							.work_groups = {static_cast<uint32_t>(grid_size.x + 15) / 16,
											static_cast<uint32_t>(grid_size.y + 15) / 16,
																	 1}});
				pipeline_map.emplace(
						"Cells",
						Graphics{.shaders = {"Vert", "Frag"},
							 .vertex_attributes = World::Cell::get_attribute_description(),
							 .vertex_bindings = World::Cell::get_binding_description()});
				pipeline_map.emplace(
						"Landscape",
						Graphics{.shaders = {"Vert", "Frag"},
							 .vertex_attributes = World::Grid::get_attribute_description(),
						 .vertex_bindings = World::Grid::get_binding_description()});
				pipeline_map.emplace(
						"LandscapeWireFrame",
						Graphics{.shaders = {"LandscapeVert", "Tesc", "Tese", "LandscapeFrag"},
							 .vertex_attributes = World::Grid::get_attribute_description(),
						 .vertex_bindings = World::Grid::get_binding_description()});
				pipeline_map.emplace("Texture",
									 Graphics{.shaders = {"Vert", "Frag"},
									.vertex_attributes = Shape::get_attribute_description(),
									.vertex_bindings = Shape::get_binding_description()});
				pipeline_map.emplace("Water",
									 Graphics{.shaders = {"Vert", "Frag"},
									.vertex_attributes = Shape::get_attribute_description(),
									.vertex_bindings = Shape::get_binding_description()});
				pipeline_map.emplace(
						"PostFX",
						Compute{
								.shaders = {"Comp"},
					.work_groups = {static_cast<uint32_t>(swapchain_extent.width + 15) / 16,
						static_cast<uint32_t>(swapchain_extent.height + 15) / 16,
															 1}});
			}

			compile_shaders();
			create_pipelines(render_pass, graphics_layout, compute_layout, msaa_samples);
		}

		void refresh_dynamic_work_groups(const Vec2UintFast16 grid_size,
																		 const VkExtent2D &swapchain_extent) {
			const auto &runtime_definitions = CE::Runtime::get_pipeline_definitions();
			for (auto &[pipeline_name, variant] : pipeline_map) {
				if (!std::holds_alternative<Compute>(variant)) {
					continue;
				}

				auto &compute = std::get<Compute>(variant);
				if (runtime_definitions.empty()) {
					compute.work_groups = default_work_groups(pipeline_name, grid_size, swapchain_extent);
					continue;
				}

				const auto definition_it = runtime_definitions.find(pipeline_name);
				if (definition_it == runtime_definitions.end()) {
					continue;
				}

				const CE::Runtime::PipelineDefinition &definition = definition_it->second;
				const bool uses_dynamic_groups = definition.work_groups[0] == 0 ||
																				 definition.work_groups[1] == 0 ||
																				 definition.work_groups[2] == 0;
				compute.work_groups = uses_dynamic_groups
																	? default_work_groups(pipeline_name,
																												grid_size,
																												swapchain_extent)
																	: definition.work_groups;
			}
		}
	};

	ComputeLayout compute;
	GraphicsLayout graphics;
	Render render;
	Configuration config;
};
