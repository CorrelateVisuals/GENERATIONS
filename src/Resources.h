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
      VkDescriptorBufferInfo bufferInfo{
          .buffer = buffer.buffer, .range = sizeof(World::UniformBufferObject)};
      info.currentFrame = bufferInfo;

      VkWriteDescriptorSet descriptorWrite{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstBinding = setLayoutBinding.binding,
          .descriptorCount = setLayoutBinding.descriptorCount,
          .descriptorType = setLayoutBinding.descriptorType,
          .pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.currentFrame)};

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
    void createDescriptorWrite(const size_t quantity) {
      VkDescriptorBufferInfo bufferInfo{
          .buffer = bufferIn.buffer,
          .offset = 0,
          .range = sizeof(World::Cell) * quantity};
      descriptorInfos[index].first = bufferInfo;
      bufferInfo.buffer = bufferOut.buffer;
      descriptorInfos[index + 1].first = bufferInfo;

      /*info.currentFrame = bufferInfo;
      bufferInfo.buffer = bufferOut.buffer;
      info.previousFrame = bufferInfo;

      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkWriteDescriptorSet descriptorWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = static_cast<uint32_t>(i ? setLayoutBinding.binding + 1
                                                  : setLayoutBinding.binding),
            .descriptorCount = setLayoutBinding.descriptorCount,
            .descriptorType = setLayoutBinding.descriptorType,
            .pBufferInfo =
                &std::get<VkDescriptorBufferInfo>(info.currentFrame)};

        descriptorWrites[i][index] = descriptorWrite;
        descriptorWrite.dstBinding = static_cast<uint32_t>(
            i ? setLayoutBinding.binding : setLayoutBinding.binding + 1);
        descriptorWrite.pBufferInfo =
            &std::get<VkDescriptorBufferInfo>(info.previousFrame);
        descriptorWrites[i][index + 1] = descriptorWrite;
      }*/
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
    void createDescriptorWrite() {
      VkDescriptorImageInfo imageInfo{
          .sampler = textureImage.sampler,
          .imageView = textureImage.view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      info.currentFrame = imageInfo;

      VkWriteDescriptorSet descriptorWrite{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstBinding = setLayoutBinding.binding,
          .descriptorCount = setLayoutBinding.descriptorCount,
          .descriptorType = setLayoutBinding.descriptorType,
          .pImageInfo = &std::get<VkDescriptorImageInfo>(info.currentFrame)};

      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        descriptorWrites[i][index] = descriptorWrite;
      }
    }

   private:
    struct TextureImage : public CE::Image {
      TextureImage() { path = Lib::path("assets/Avatar.PNG"); }
    } textureImage;
  } sampler;

  class StorageImage : public CE::Descriptor {
   public:
    StorageImage(std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
    void createDescriptorWrite(
        std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images) {
      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                        .imageView = images[i].view,
                                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

        !i ? info.currentFrame = imageInfo : info.previousFrame = imageInfo;

        VkWriteDescriptorSet descriptorWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = setLayoutBinding.binding,
            .descriptorCount = setLayoutBinding.descriptorCount,
            .descriptorType = setLayoutBinding.descriptorType,
            .pImageInfo = &std::get<VkDescriptorImageInfo>(
                !i ? info.currentFrame : info.previousFrame)};

        descriptorWrites[i][index] = descriptorWrite;
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
