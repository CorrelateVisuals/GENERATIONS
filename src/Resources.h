#pragma once
#include "vulkan/vulkan.h"

#include "Mechanics.h"
#include "Pipelines.h"
#include "World.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

class VulkanMechanics;
class Pipelines;

class Resources {
 public:
  Resources(VulkanMechanics& mechanics);
  ~Resources();

  World world{};

  const std::unordered_map<Geometry*, VkVertexInputRate> vertexBuffers = {
      {&world.landscape, VK_VERTEX_INPUT_RATE_INSTANCE},
      {&world.rectangle, VK_VERTEX_INPUT_RATE_INSTANCE},
      {&world.cube, VK_VERTEX_INPUT_RATE_VERTEX}};

  CE::Image depthImage{};
  CE::Image msaaImage{};
  CE::Image textureImage{};

  struct Uniform : public CE::Descriptor {
    CE::Buffer buffer{};
    Uniform() {
      VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      setLayoutBinding.binding = 0;
      setLayoutBinding.descriptorType = type;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags =
          VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = type;
      poolSize.descriptorCount = static_cast<uint32_t>(2);
      poolSizes.push_back(poolSize);

      for (uint_fast8_t i = 0; i < 2; i++) {
        bufferInfo.buffer = buffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(World::UniformBufferObject);
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // writeSet.dstSet = sets[i];
        writeSet.dstBinding = 0;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = type;
        writeSet.pBufferInfo = &bufferInfo;
      }
    }
  } uniform;

  struct ShaderStorage : public CE::Descriptor {
    CE::Buffer bufferIn{};
    CE::Buffer bufferOut{};
    ShaderStorage(uint32_t range) {
      setLayoutBinding.binding = 1;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      setLayoutBindings.push_back(setLayoutBinding);
      setLayoutBinding.binding = 2;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2) * 2;
      poolSizes.push_back(poolSize);

      bufferInfo.buffer = bufferIn.buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = range;

      bufferInfo.buffer = bufferOut.buffer;
    }
  } shaderStorage;

  struct ImageSampler : public CE::Descriptor {
    CE::Buffer buffer{};
    ImageSampler(CE::Image image) {
      setLayoutBinding.binding = 3;
      setLayoutBinding.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2);
      poolSizes.push_back(poolSize);

      imageInfo.sampler = image.sampler;
      imageInfo.imageView = image.view;
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
  } sampler;

  struct StorageImage : public CE::Descriptor {
    CE::Buffer buffer{};
    StorageImage(std::vector<CE::Image>& images) {
      setLayoutBinding.binding = 4;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      setLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2);
      poolSizes.push_back(poolSize);

      for (uint_fast8_t i = 0; i < images.size(); i++) {
        imageInfo.sampler = VK_NULL_HANDLE;
        imageInfo.imageView = images[i].view;  // TODO ERROR FOR VECTOR SIZE
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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

  struct CommandBuffers : public CE::Commands {
    CommandBuffers(const CE::Queues::FamilyIndices& familyIndices) {
      createPool(familyIndices);
    };
    std::vector<VkCommandBuffer> graphics{};
    std::vector<VkCommandBuffer> compute{};
  } command;

 public:
  void setupResources(Pipelines& _pipelines);

  void createFramebuffers(Pipelines& _pipelines);

  void updateUniformBuffer(uint32_t currentImage);

  void createDescriptorSets();

  void recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex,
                                   Pipelines& _pipelines);
  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer,
                                  Pipelines& _pipelines);

 private:
  VulkanMechanics& _mechanics;

  void createDescriptorPool();
  void allocateDescriptorSets();

  void createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers,
                            const int size);

  void createVertexBuffers(
      const std::unordered_map<Geometry*, VkVertexInputRate>& buffers);
  void createVertexBuffer(CE::Buffer& buffer, const auto& vertices);
  void createIndexBuffer(CE::Buffer& buffer, const auto& indices);
  void createShaderStorageBuffers();
  void createUniformBuffers();
  void setPushConstants();
};
