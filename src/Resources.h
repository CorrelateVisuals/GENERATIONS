#pragma once
#include "vulkan/vulkan.h"

#include "pipelines/ShaderAccess.h"
#include "World.h"
#include "vulkan_base/VulkanDescriptor.h"
#include "vulkan_base/VulkanPipeline.h"
#include "vulkan_base/VulkanResources.h"
#include "vulkan_base/VulkanSync.h"
#include "core/RuntimeConfig.h"

#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class VulkanMechanics;

class Resources {
public:
	Resources(VulkanMechanics &mechanics, const CE::Runtime::TerrainSettings &terrain_settings);
	Resources(const Resources &) = delete;
	Resources &operator=(const Resources &) = delete;
	Resources(Resources &&) = delete;
	Resources &operator=(Resources &&) = delete;
	~Resources();

	class UniformBuffer : public CE::Descriptor {
	public:
		UniformBuffer(CE::DescriptorInterface &interface, World::UniformBufferObject &u);
		void update(World &world, const VkExtent2D extent);

	private:
		CE::Buffer buffer;
		World::UniformBufferObject &ubo;
		void create_buffer();
		void create_descriptor_write(CE::DescriptorInterface &interface);
	};

	class StorageBuffer : public CE::Descriptor {
	public:
		CE::Buffer buffer_in;
		CE::Buffer buffer_out;

		StorageBuffer(CE::DescriptorInterface &descriptor_interface,
									const CE::CommandInterface &command_interface,
									const auto &object,
									const size_t quantity);

	private:
		void create(const CE::CommandInterface &command_interface,
								const auto &object,
								const size_t quantity);
		void create_descriptor_write(CE::DescriptorInterface &interface, const size_t quantity);
	};

	class ImageSampler : public CE::Descriptor {
	public:
		ImageSampler(CE::DescriptorInterface &interface,
								 const CE::CommandInterface &command_interface,
								 const std::string &texture_path);

	private:
		void create_descriptor_write(CE::DescriptorInterface &interface);
		CE::Image texture_image;
	};

	class StorageImage : public CE::Descriptor {
	public:
		StorageImage(CE::DescriptorInterface &interface,
								 std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images);
		void create_descriptor_write(CE::DescriptorInterface &interface,
																 std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images);
	};
	CE::ShaderAccess::CommandResources
			commands;
	CE::CommandInterface command_interface;
	CE::PushConstants push_constant;

	World world;

	CE::DescriptorInterface descriptor_interface;

	CE::Image depth_image;
	CE::Image msaa_image;

	UniformBuffer uniform;
	StorageBuffer shader_storage;

	ImageSampler sampler;
	StorageImage storage_image;

	bool startup_seed_pending = true;
};
