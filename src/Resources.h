#pragma once
#include "vulkan/vulkan.h"

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
    void createDescriptorWrite() {
      static size_t index = descriptorWriteIndex;
      descriptorWriteIndex++;

      VkDescriptorBufferInfo bufferInfo{
          .buffer = buffer.buffer, .range = sizeof(World::UniformBufferObject)};
      info = bufferInfo;

      VkWriteDescriptorSet descriptorWrite{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstBinding = setLayoutBinding.binding,
          .descriptorCount = setLayoutBinding.descriptorCount,
          .descriptorType = setLayoutBinding.descriptorType,
          .pBufferInfo = &std::get<VkDescriptorBufferInfo>(info)};

      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        descriptorWrites[i][index] = descriptorWrite;
      }
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
      static size_t index = descriptorWriteIndex;
      descriptorWriteIndex += 2;

      VkDescriptorBufferInfo bufferInfo{
          .buffer = bufferIn.buffer,
          .offset = 0,
          .range = sizeof(World::Cell) * quantity};
      descriptorInfos[index].first = bufferInfo;
      bufferInfo.buffer = bufferOut.buffer;
      descriptorInfos[index + 1].first = bufferInfo;

      VkWriteDescriptorSet descriptorWrite{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = nullptr,
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &std::get<VkDescriptorBufferInfo>(info)};

      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        descriptorWrites[i][index] = descriptorWrite;
      }
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
      static size_t index = descriptorWriteIndex;
      descriptorWriteIndex++;
      VkDescriptorImageInfo imageInfo{
          .sampler = textureImage.sampler,
          .imageView = textureImage.view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      descriptorInfos[index].first = imageInfo;
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
      static size_t index = descriptorWriteIndex;
      descriptorWriteIndex += 2;

      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                        .imageView = images[i].view,
                                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
        descriptorInfos[index + i].first = imageInfo;
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

 private:
  static std::vector<
      std::variant<UniformBuffer, ShaderStorage, ImageSampler, StorageImage>>
      descriptors;
};
