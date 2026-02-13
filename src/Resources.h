#pragma once
#include "vulkan/vulkan.h"

#include "ShaderAccess.h"
#include "World.h"
#include "base/VulkanDescriptor.h"
#include "base/VulkanPipeline.h"
#include "base/VulkanResources.h"
#include "base/VulkanSync.h"

#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class VulkanMechanics;

class Resources {
 public:
  Resources(VulkanMechanics& mechanics);
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
    UniformBuffer(CE::DescriptorInterface& interface,
                  World::UniformBufferObject& u);
    void update(World& world, const VkExtent2D extent);

   private:
    CE::Buffer buffer;
    World::UniformBufferObject& ubo;
    void createBuffer();
    void createDescriptorWrite(CE::DescriptorInterface& interface);
  };

  class StorageBuffer : public CE::Descriptor {
   public:
    CE::Buffer bufferIn;
    CE::Buffer bufferOut;

    StorageBuffer(CE::DescriptorInterface& descriptorInterface,
                  const CE::CommandInterface& commandInterface,
                  const auto& object,
                  const size_t quantity);

   private:
    void create(const CE::CommandInterface& commandInterface,
                const auto& object,
                const size_t quantity);
    void createDescriptorWrite(CE::DescriptorInterface& interface,
                               const size_t quantity);
  };

  class ImageSampler : public CE::Descriptor {
   public:
    ImageSampler(CE::DescriptorInterface& interface,
                 const CE::CommandInterface& commandInterface,
                 const std::string& texturePath);

   private:
    void createDescriptorWrite(CE::DescriptorInterface& interface);
    CE::Image textureImage;
  };

  class StorageImage : public CE::Descriptor {
   public:
    StorageImage(CE::DescriptorInterface& interface,
                 std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
    void createDescriptorWrite(
        CE::DescriptorInterface& interface,
        std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
  };
  // GPU Interface
  CE::ShaderAccess::CommandResources
      commands;  // virtual function to record command buffers
  CE::CommandInterface commandInterface;  // interface for command buffers
  CE::PushConstants push_constant;

  // Scene
  World world;  // World objects, light, _camera

  CE::DescriptorInterface descriptorInterface;

  // Images
  CE::Image depthImage;
  CE::Image msaaImage;

  // Descriptors
  UniformBuffer uniform;        // UniformParameters world details
  StorageBuffer shaderStorage;  // FeedbackLoop compute shader

  // Image Descriptors
  ImageSampler sampler;       // Texture vertex shader
  StorageImage storageImage;  //  PostFX compute shader
};
