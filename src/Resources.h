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

  World world;

  const std::unordered_map<Geometry*, VkVertexInputRate> vertexBuffers = {
      {&world.landscape, VK_VERTEX_INPUT_RATE_INSTANCE},
      {&world.rectangle, VK_VERTEX_INPUT_RATE_INSTANCE},
      {&world.cube, VK_VERTEX_INPUT_RATE_VERTEX}};

  struct DepthImage : public CE::Image {
  } depthImage;
  struct MultiSamplingImage : public CE::Image {
  } msaaImage;
  struct Texture : public CE::Image {
  } textureImage;

  static std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  struct Uniform : public CE::Descriptor::SetLayout {
    CE::Buffer buffer;
    Uniform();
  } uniform;

  struct ShaderStorage : public CE::Descriptor::SetLayout {
    CE::Buffer bufferIn;
    CE::Buffer bufferOut;
    ShaderStorage();
  } shaderStorage;

  struct ImageSampler : public CE::Descriptor::SetLayout {
    CE::Buffer buffer;
    ImageSampler();
  } sampler;

  struct StorageImage : public CE::Descriptor::SetLayout {
    CE::Buffer buffer;
    StorageImage();
  } storageImage;

  struct PushConstants {
    VkShaderStageFlags shaderStage = {VK_SHADER_STAGE_COMPUTE_BIT};
    uint32_t count = 1;
    uint32_t offset = 0;
    uint32_t size = 128;
    std::array<uint64_t, 32> data;
  } pushConstants;

  struct CommandBuffers : public CE::Commands {
    std::vector<VkCommandBuffer> graphics;
    std::vector<VkCommandBuffer> compute;
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
