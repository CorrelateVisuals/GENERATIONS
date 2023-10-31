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

  // std::unordered_map<std::string, CE::Descriptor> newResourceMap;

  static std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  struct Uniform : public CE::Descriptor {
    CE::Buffer buffer{};
    Uniform() {
      setLayoutBinding.binding = 0;
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBinding.stageFlags =
          VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
      descriptorSetLayoutBindings.push_back(setLayoutBinding);
    }
  } uniform;

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

  std::vector<VkDescriptorPoolSize> poolSizes{
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = static_cast<uint32_t>(2)},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = static_cast<uint32_t>(2) * 2},
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = static_cast<uint32_t>(2)},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .descriptorCount = static_cast<uint32_t>(2)}};

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
