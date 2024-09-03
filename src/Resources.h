#pragma once
#include "vulkan/vulkan.h"

#include "BaseClasses.h"
#include "Mechanics.h"
#include "Pipelines.h"
#include "World.h"

#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class Resources {
 public:
  Resources(VulkanMechanics& mechanics, Pipelines& pipelines);
  ~Resources();

  struct CommandResources : public CE::CommandBuffers {
    CommandResources(const CE::Queues::FamilyIndices& familyIndices);
    void recordComputeCommandBuffer(CE::DescriptorInterface& interface,
                                    Resources& resources,
                                    Pipelines& pipelines,
                                    const uint32_t imageIndex) override;
    void recordGraphicsCommandBuffer(CE::DescriptorInterface& interface,
                                     CE::Swapchain& swapchain,
                                     Resources& resources,
                                     Pipelines& pipelines,
                                     const uint32_t imageIndex) override;
  };

  class UniformBuffer : public CE::Descriptor {
   public:
    UniformBuffer(World::UniformBufferObject& u,
                  CE::DescriptorInterface& interface);
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

    StorageBuffer(const CE::CommandInterface& commandData,
                  const auto& object,
                  const size_t quantity);

   private:
    void create(const CE::CommandInterface& commandData,
                const auto& object,
                const size_t quantity);
    void createDescriptorWrite(const size_t quantity,
                               CE::DescriptorInterface& interface);
  };

  class ImageSampler : public CE::Descriptor {
   public:
    ImageSampler(CE::DescriptorInterface& interface,
                 const CE::CommandInterface& commandData,
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
  CommandResources commands;  // virtual function to record command buffers
  CE::CommandInterface commandInterface;  // interface for command buffers
  CE::PushConstants pushConstant;

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
