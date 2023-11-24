#pragma once
#include "vulkan/vulkan.h"

#include "Mechanics.h"
#include "Pipelines.h"
#include "World.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

class Resources {
 public:
  Resources(VulkanMechanics& mechanics, Pipelines& pipelines);
  ~Resources();

  std::vector<CE::Descriptor> descriptors{};

  struct Commands : public CE::CommandBuffers {
    Commands(const CE::Queues::FamilyIndices& familyIndices);
    void recordComputeCommandBuffer(Resources& resources,
                                    Pipelines& pipelines,
                                    const uint32_t imageIndex) override;
    void recordGraphicsCommandBuffer(CE::Swapchain& swapchain,
                                     Resources& resources,
                                     Pipelines& pipelines,
                                     const uint32_t imageIndex) override;
  } commands;

  World world;

  struct DepthImage : public CE::Image {
    DepthImage(const VkExtent2D extent, const VkFormat format);
  } depthImage;

  struct MultisamplingImage : public CE::Image {
    MultisamplingImage(const VkExtent2D extent, const VkFormat format);
  } msaaImage;

  class UniformBuffer : public CE::Descriptor {
   public:
    CE::Buffer buffer;
    UniformBuffer();
    void update(World& world, const VkExtent2D extent);
    void createDescriptorInfo() {
      VkDescriptorBufferInfo bufferInfo{
          .buffer = buffer.buffer,
          .offset = 0,
          .range = sizeof(World::UniformBufferObject)};
      descriptorInfos.push_back(bufferInfo);
    };

   private:
    World::UniformBufferObject object;
    void create();
  } uniform;

  class ShaderStorage : public CE::Descriptor {
   public:
    CE::Buffer bufferIn;
    CE::Buffer bufferOut;
    ShaderStorage(VkCommandBuffer& commandBuffer,
                  const VkCommandPool& commandPool,
                  const VkQueue& queue,
                  const auto& object,
                  const size_t quantity);
    void createDescriptorInfo(const size_t quantity) {
      VkDescriptorBufferInfo bufferInfo{
          .buffer = bufferIn.buffer,
          .offset = 0,
          .range = sizeof(World::Cell) * quantity};
      descriptorInfos.push_back(bufferInfo);
      bufferInfo.buffer = bufferOut.buffer;
      descriptorInfos.push_back(bufferInfo);
    };

   private:
    void create(VkCommandBuffer& commandBuffer,
                const VkCommandPool& commandPool,
                const VkQueue& queue,
                const auto& object,
                const size_t quantity);
  } shaderStorage;

  class ImageSampler : public CE::Descriptor {
   public:
    ImageSampler(VkCommandBuffer& commandBuffer,
                 VkCommandPool& commandPool,
                 const VkQueue& queue);
    void createDescriptorInfo() {
      VkDescriptorImageInfo imageInfo{
          .sampler = textureImage.sampler,
          .imageView = textureImage.view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      descriptorInfos.push_back(imageInfo);
    }

   private:
    struct TextureImage : public CE::Image {
      TextureImage() { path = Lib::path("assets/Avatar.PNG"); }
    } textureImage;
  } sampler;

  class StorageImage : public CE::Descriptor {
   public:
    StorageImage(std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);

    void createDescriptorInfo(
        std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images) {
      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                        .imageView = images[i].view,
                                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
        descriptorInfos.push_back(imageInfo);
      }
    }
  } storageImage;

  struct PushConstants : public CE::PushConstants {
    PushConstants() {
      shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
      count = 1;
      offset = 0;
      size = 128;
      data.fill(0);
    }
  } pushConstants;
};
