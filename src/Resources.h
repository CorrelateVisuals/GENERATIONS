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

  struct UniformBuffer : public CE::Descriptor {
    CE::Buffer buffer;
    World::UniformBufferObject object;

    UniformBuffer() {
      VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      setLayoutBinding.binding = 0;
      setLayoutBinding.descriptorType = type;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags =
          VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = type;
      poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
      poolSizes.push_back(poolSize);

      create();

      bufferInfo.buffer = buffer.buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(World::UniformBufferObject);
      descriptorInfos.push_back(bufferInfo);
    }
    void create();
    void update(World& world, const VkExtent2D extent);
  } uniform;

  struct ShaderStorage : public CE::Descriptor {
    CE::Buffer bufferIn;
    CE::Buffer bufferOut;
    ShaderStorage(VkCommandBuffer& commandBuffer,
                  const VkCommandPool& commandPool,
                  const VkQueue& queue,
                  const auto& object,
                  const size_t quantity) {
      setLayoutBinding.binding = 1;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      setLayoutBindings.push_back(setLayoutBinding);
      setLayoutBinding.binding = 2;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
      poolSizes.push_back(poolSize);

      create(commandBuffer, commandPool, queue, object, quantity);

      bufferInfo.buffer = bufferIn.buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(World::Cell) * quantity;
      descriptorInfos.push_back(bufferInfo);
      bufferInfo.buffer = bufferOut.buffer;
      descriptorInfos.push_back(bufferInfo);
    }
    void create(VkCommandBuffer& commandBuffer,
                const VkCommandPool& commandPool,
                const VkQueue& queue,
                const auto& object,
                const size_t quantity);
  } shaderStorage;

  struct ImageSampler : public CE::Descriptor {
    CE::Buffer buffer;

    struct TextureImage : public CE::Image {
      TextureImage() { path = Lib::path("assets/Avatar.PNG"); }
    } textureImage;

    ImageSampler(VkCommandBuffer& commandBuffer,
                 VkCommandPool& commandPool,
                 const VkQueue& queue) {
      setLayoutBinding.binding = 3;
      setLayoutBinding.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
      poolSizes.push_back(poolSize);

      textureImage.loadTexture(textureImage.path, VK_FORMAT_R8G8B8A8_SRGB,
                               commandBuffer, commandPool, queue);
      textureImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);
      textureImage.createSampler();

      imageInfo.sampler = textureImage.sampler;
      imageInfo.imageView = textureImage.view;
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      descriptorInfos.push_back(imageInfo);
    }
  } sampler;

  struct StorageImage : public CE::Descriptor {
    CE::Buffer buffer;
    StorageImage(std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images) {
      setLayoutBinding.binding = 4;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
      poolSizes.push_back(poolSize);

      for (uint_fast8_t i = 0; i < images.size(); i++) {
        imageInfo.sampler = VK_NULL_HANDLE;
        imageInfo.imageView = images[i].view;  // TODO ERROR FOR VECTOR SIZE
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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
