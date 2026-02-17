#pragma once

// GPU resource aggregate for per-frame rendering and compute.
// Exists to keep descriptor/buffer/image setup co-located with world-owned data.
#include "vulkan/vulkan.h"

#include "vulkan_pipelines/ShaderAccess.h"
#include "world/World.h"
#include "vulkan_base/VulkanBaseDescriptor.h"
#include "vulkan_base/VulkanBasePipeline.h"
#include "vulkan_base/VulkanBaseResources.h"
#include "vulkan_base/VulkanBaseSync.h"
#include "world/RuntimeConfig.h"

#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class VulkanMechanics;

class VulkanResources {
public:
	VulkanResources(VulkanMechanics &mechanics, const CE::Runtime::TerrainSettings &terrain_settings);
	VulkanResources(const VulkanResources &) = delete;
	VulkanResources &operator=(const VulkanResources &) = delete;
	VulkanResources(VulkanResources &&) = delete;
	VulkanResources &operator=(VulkanResources &&) = delete;
	~VulkanResources();

	class UniformBuffer : public CE::BaseDescriptor {
	public:
		UniformBuffer(CE::BaseDescriptorInterface &interface, World::UniformBufferObject &u);
		void update(World &world, const VkExtent2D extent);

	private:
		CE::BaseBuffer buffer;
		World::UniformBufferObject &ubo;
		void create_buffer();
		void create_descriptor_write(CE::BaseDescriptorInterface &interface);
	};

	class StorageBuffer : public CE::BaseDescriptor {
	public:
		CE::BaseBuffer buffer_in;
		CE::BaseBuffer buffer_out;

		StorageBuffer(CE::BaseDescriptorInterface &descriptor_interface,
									const CE::BaseCommandInterface &command_interface,
									const auto &object,
									const size_t quantity);

	private:
		void create(const CE::BaseCommandInterface &command_interface,
								const auto &object,
								const size_t quantity);
		void create_descriptor_write(CE::BaseDescriptorInterface &interface, const size_t quantity);
	};

	class ImageSampler : public CE::BaseDescriptor {
	public:
		ImageSampler(CE::BaseDescriptorInterface &interface,
								 const CE::BaseCommandInterface &command_interface,
								 const std::string &texture_path);

	private:
		void create_descriptor_write(CE::BaseDescriptorInterface &interface);
		CE::BaseImage texture_image;
	};

	class StorageImage : public CE::BaseDescriptor {
	public:
		StorageImage(CE::BaseDescriptorInterface &interface,
								 std::array<CE::BaseImage, MAX_FRAMES_IN_FLIGHT> &images);
		void create_descriptor_write(CE::BaseDescriptorInterface &interface,
																 std::array<CE::BaseImage, MAX_FRAMES_IN_FLIGHT> &images);
	};
	CE::ShaderAccess::CommandResources
			commands;
	CE::BaseCommandInterface command_interface;
	CE::BasePushConstants push_constant;

	World world;

	CE::BaseDescriptorInterface descriptor_interface;

	CE::BaseImage depth_image;
	CE::BaseImage msaa_image;

	UniformBuffer uniform;
	StorageBuffer shader_storage;

	ImageSampler sampler;
	StorageImage storage_image;

	bool startup_seed_pending = true;
};
