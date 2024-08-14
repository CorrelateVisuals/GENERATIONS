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
    void recordComputeCommandBuffer(Resources& resources,
                                    Pipelines& pipelines,
                                    const uint32_t imageIndex) override;
    void recordGraphicsCommandBuffer(CE::Swapchain& swapchain,
                                     Resources& resources,
                                     Pipelines& pipelines,
                                     const uint32_t imageIndex) override;
  };

  class UniformBuffer : public CE::Descriptor {
   public:
    UniformBuffer(World::UniformBufferObject& u);
    void update(World& world, const VkExtent2D extent);

   private:
    CE::Buffer buffer;
    World::UniformBufferObject& ubo;
    void createBuffer();
    void createDescriptorWrite();
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
    void createDescriptorWrite(const size_t quantity);
  };

  class ImageSampler : public CE::Descriptor {
   public:
    ImageSampler(const CE::CommandInterface& commandData,
                 const std::string& texturePath);

   private:
    void createDescriptorWrite();
    CE::Image textureImage;
  };

  class StorageImage : public CE::Descriptor {
   public:
    StorageImage(std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
    void createDescriptorWrite(
        std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
  };
  // GPU Interface
  CommandResources commands;
  CE::CommandInterface commandInterface;
  CE::PushConstants pushConstant;

  // Scene
  World world;  // World objects, light, camera

  // Images
  CE::Image depthImage;
  CE::Image msaaImage;

  // Descriptors
  UniformBuffer uniform;        // UniformParameters
  StorageBuffer shaderStorage;  // FeedbackLoop

  // Image Descriptors
  ImageSampler sampler;       // Texture
  StorageImage storageImage;  //  PostFX
};
