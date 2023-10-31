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

  static std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  static std::vector<VkDescriptorPoolSize> newPoolSizes;

  struct Uniform : public CE::Descriptor {
    CE::Buffer buffer{};
    Uniform() {
      setLayoutBinding.binding = 0;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags =
          VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
      descriptorSetLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2);
      newPoolSizes.push_back(poolSize);

      //  bufferInfo.buffer = this->buffer;
      //  bufferInfo.offset = 0;
      //  bufferInfo.range = sizeof(World::UniformBufferObject);

      //  std::vector<VkWriteDescriptorSet> descriptorWrites{
      //{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      // .dstSet = descriptor.sets[i],
      // .dstBinding = 0,
      // .dstArrayElement = 0,
      // .descriptorCount = 1,
      // .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      // .pBufferInfo = &uniformBufferInfo},
    }
  } uniform;



  //VkDescriptorBufferInfo uniformBufferInfo{
  //      .buffer = uniform.buffer.buffer,
  //      .offset = 0,
  //      .range = sizeof(World::UniformBufferObject) };

  //VkDescriptorBufferInfo storageBufferInfoLastFrame{
  //    .buffer = shaderStorage.bufferIn.buffer,
  //    .offset = 0,
  //    .range = sizeof(World::Cell) * world.grid.size.x * world.grid.size.y };

  //VkDescriptorBufferInfo storageBufferInfoCurrentFrame{
  //    .buffer = shaderStorage.bufferOut.buffer,
  //    .offset = 0,
  //    .range = sizeof(World::Cell) * world.grid.size.x * world.grid.size.y };

  //VkDescriptorImageInfo imageInfo{
  //    .sampler = textureImage.sampler,
  //    .imageView = textureImage.view,
  //    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

  //VkDescriptorImageInfo swapchainImageInfo{
  //    .sampler = VK_NULL_HANDLE,
  //    .imageView = _mechanics.swapchain.images[i].view,
  //    .imageLayout = VK_IMAGE_LAYOUT_GENERAL };

  //std::vector<VkWriteDescriptorSet> descriptorWrites{
  //    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //     .dstSet = descriptor.sets[i],
  //     .dstBinding = 0,
  //     .dstArrayElement = 0,
  //     .descriptorCount = 1,
  //     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  //     .pBufferInfo = &uniformBufferInfo},

  //    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //     .dstSet = descriptor.sets[i],
  //     .dstBinding = static_cast<uint32_t>(i ? 2 : 1),
  //     .dstArrayElement = 0,
  //     .descriptorCount = 1,
  //     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
  //     .pBufferInfo = &storageBufferInfoLastFrame},

  //    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //     .dstSet = descriptor.sets[i],
  //     .dstBinding = static_cast<uint32_t>(i ? 1 : 2),
  //     .dstArrayElement = 0,
  //     .descriptorCount = 1,
  //     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
  //     .pBufferInfo = &storageBufferInfoCurrentFrame},

  //    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //     .dstSet = descriptor.sets[i],
  //     .dstBinding = 3,
  //     .dstArrayElement = 0,
  //     .descriptorCount = 1,
  //     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  //     .pImageInfo = &imageInfo},

  //    {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //     .dstSet = descriptor.sets[i],
  //     .dstBinding = 4,
  //     .dstArrayElement = 0,
  //     .descriptorCount = 1,
  //     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  //     .pImageInfo = &swapchainImageInfo},

  struct ShaderStorage : public CE::Descriptor {
    CE::Buffer bufferIn{};
    CE::Buffer bufferOut{};
    ShaderStorage() {
      setLayoutBinding.binding = 1;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      descriptorSetLayoutBindings.push_back(setLayoutBinding);
      setLayoutBinding.binding = 2;
      descriptorSetLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2) * 2;
      newPoolSizes.push_back(poolSize);
    }
  } shaderStorage;

  struct ImageSampler : public CE::Descriptor {
    CE::Buffer buffer{};
    ImageSampler() {
      setLayoutBinding.binding = 3;
      setLayoutBinding.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      descriptorSetLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2);
      newPoolSizes.push_back(poolSize);
    }
  } sampler;

  struct StorageImage : public CE::Descriptor {
    CE::Buffer buffer{};
    StorageImage() {
      setLayoutBinding.binding = 4;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      descriptorSetLayoutBindings.push_back(setLayoutBinding);

      poolSize.type = setLayoutBinding.descriptorType;
      poolSize.descriptorCount = static_cast<uint32_t>(2);
      newPoolSizes.push_back(poolSize);
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
    std::vector<VkCommandBuffer> graphics{};
    std::vector<VkCommandBuffer> compute{};
  } command;

  struct DescriptorSets : CE::Descriptor {
  } descriptor;

 public:
  void setupResources(Pipelines& _pipelines);

  void createFramebuffers(Pipelines& _pipelines);

  void updateUniformBuffer(uint32_t currentImage);

  void createDescriptorSetLayout(
      const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);
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
