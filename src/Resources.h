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

  struct Commands : public CE::CommandBuffers {
    Commands(const CE::Queues::FamilyIndices& familyIndices);
    void recordComputeCommandBuffer(Resources& resources,
                                    Pipelines& pipelines,
                                    const uint32_t imageIndex) override;
    void recordGraphicsCommandBuffer(CE::Swapchain& swapchain,
                                     Resources& resources,
                                     Pipelines& pipelines,
                                     const uint32_t imageIndex) override;
  };

  struct CommandInterface {
    VkCommandBuffer& commandBuffer;
    const VkCommandPool& commandPool;
    const VkQueue& queue;

    CommandInterface(VkCommandBuffer& cmdBuffer,
                const VkCommandPool& cmdPool,
                const VkQueue& q)
        : commandBuffer(cmdBuffer), commandPool(cmdPool), queue(q) {}
  };

  struct DepthImage : public CE::Image {
    DepthImage(const VkExtent2D extent, const VkFormat format);
  };

  struct MultisamplingImage : public CE::Image {
    MultisamplingImage(const VkExtent2D extent, const VkFormat format);
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

    StorageBuffer(const CommandInterface& commandData,
                  const auto& object,
                  const size_t quantity);

   private:
    void create(const CommandInterface& commandData,
                const auto& object,
                const size_t quantity);
    void createDescriptorWrite(const size_t quantity);
  };

  class ImageSampler : public CE::Descriptor {
   public:
    ImageSampler(const CommandInterface& commandData,
                 const std::string& texturePath);

   private:
    void createDescriptorWrite();

    struct TextureImage : public CE::Image {
      TextureImage(const std::string& texturePath) { path = texturePath; }
    };

    TextureImage textureImage;
  };

  class StorageImage : public CE::Descriptor {
   public:
    StorageImage(std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
    void createDescriptorWrite(
        std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
  };

  class PushConstants : public CE::PushConstants {
   public:
    PushConstants(VkShaderStageFlags stage,
                  uint32_t dataSize,
                  uint32_t dataOffset = 0);
  };

  Commands commands;

  World world;
  DepthImage depthImage;
  MultisamplingImage msaaImage;
  UniformBuffer uniform;

  CommandInterface commandInterface;

  StorageBuffer shaderStorage;
  ImageSampler sampler;
  StorageImage storageImage;
  PushConstants pushConstants;
};
