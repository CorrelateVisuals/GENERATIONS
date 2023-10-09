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
    // MultiSamplingImage(VkFormat format, VkExtent2D extent) {
    //   create(extent.width, extent.height, this->sampleCount, format,
    //          VK_IMAGE_TILING_OPTIMAL,
    //          VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
    //              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    //          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->image, this->memory);
    //   this->view =
    //       CE::Image::createView(this->image, format,
    //       VK_IMAGE_ASPECT_COLOR_BIT);
    // };
  } msaaImage;
  struct Texture : public CE::Image {
  } textureImage;

  static std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  struct Uniform : CE::Descriptor::SetLayout {
    CE::Buffer buffer;
    Uniform();
  } uniform;

  struct ShaderStorage : CE::Descriptor::SetLayout {
    CE::Buffer bufferIn;
    CE::Buffer bufferOut;
    ShaderStorage();
  } shaderStorage;

  struct ImageSampler : CE::Descriptor::SetLayout {
    CE::Buffer buffer;
    ImageSampler();
  } sampler;

  struct StorageImage : CE::Descriptor::SetLayout {
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

  struct CommandBuffers {
    VkCommandPool pool;
    VkCommandBuffer singleTime;
    std::vector<VkCommandBuffer> graphics;
    std::vector<VkCommandBuffer> compute;
  } command;

  struct DescriptorSets : CE::Descriptor {
  } descriptor;

 public:
  void setupResources(Pipelines& _pipelines);
  void createFramebuffers(Pipelines& _pipelines);

  void createDescriptorSetLayout(
      const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);
  void createDescriptorSets();

  void recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex,
                                   Pipelines& _pipelines);
  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer,
                                  Pipelines& _pipelines);
  void updateUniformBuffer(uint32_t currentImage);

  void createColorResources();
  void createDepthResources();
  VkFormat findDepthFormat();

 private:
  VulkanMechanics& _mechanics;

  void createVertexBuffers(
      const std::unordered_map<Geometry*, VkVertexInputRate>& buffers);

  void createVertexBuffer(VkBuffer& buffer,
                          VkDeviceMemory& bufferMemory,
                          const auto& vertices);
  void createIndexBuffer(VkBuffer& buffer,
                         VkDeviceMemory& bufferMemory,
                         const auto& indices);

  void setPushConstants();

  void createGraphicsCommandBuffers();
  void createComputeCommandBuffers();

  void createDescriptorPool();
  void allocateDescriptorSets();

  void createShaderStorageBuffers();
  void createUniformBuffers();

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
};
