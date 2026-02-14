#pragma once
#include "vulkan/vulkan.h"

#include "ShaderAccess.h"
#include "world/World.h"
#include "base/VulkanDescriptor.h"
#include "base/VulkanPipeline.h"
#include "base/VulkanResources.h"
#include "base/VulkanSync.h"
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

  // struct CommandResources : public CE::CommandBuffers {
  //   CommandResources(const CE::Queues::FamilyIndices& familyIndices);
  //   void recordComputeCommandBuffer(Resources& resources,
  //                                   Pipelines& pipelines,
  //                                   const uint32_t imageIndex) override;
  //   void recordGraphicsCommandBuffer(CE::Swapchain& swapchain,
  //                                    Resources& resources,
  //                                    Pipelines& pipelines,
  //                                    const uint32_t imageIndex) override;
  // };

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
  // GPU Interface
  CE::ShaderAccess::CommandResources
      commands;                          // virtual function to record command buffers
  CE::CommandInterface command_interface; // interface for command buffers
  CE::PushConstants push_constant;

  // Scene
  World world; // World objects, light, _camera

  CE::DescriptorInterface descriptor_interface;

  // Images
  CE::Image depth_image;
  CE::Image msaa_image;

  // Descriptors
  UniformBuffer uniform;       // UniformParameters world details
  StorageBuffer shader_storage; // FeedbackLoop compute shader

  // Image Descriptors
  ImageSampler sampler;      // Texture vertex shader
  StorageImage storage_image; //  PostFX compute shader
};
