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

  static World world;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;

  VkBuffer vertexBufferLandscape;
  VkDeviceMemory vertexBufferMemoryLandscape;
  VkBuffer indexBufferLandscape;
  VkDeviceMemory indexBufferMemoryLandscape;

  struct PushConstants {
    VkShaderStageFlags shaderStage = {VK_SHADER_STAGE_COMPUTE_BIT};
    uint32_t count = 1;
    uint32_t offset = 0;
    uint32_t size = 128;
    std::array<uint64_t, 32> data;
  } pushConstants;

  struct Images {
    VkImage texture;
    VkDeviceMemory textureMemory;
    VkImageView textureView;
    VkSampler textureSampler;
  } image;

  struct Buffers {
    std::vector<VkBuffer> shaderStorage;
    std::vector<VkDeviceMemory> shaderStorageMemory;

    std::vector<VkBuffer> uniforms;
    std::vector<VkDeviceMemory> uniformsMemory;
    std::vector<void*> uniformsMapped;

    struct CommandBuffers {
      VkCommandPool pool;
      std::vector<VkCommandBuffer> graphic;
      std::vector<VkCommandBuffer> compute;
    } command;
  } buffers;

  struct DescriptorSets {
    VkDescriptorPool pool;
    VkDescriptorSetLayout setLayout;
    std::vector<VkDescriptorSet> sets;
  } descriptor;

 public:
  void createResources(Pipelines& _pipelines);
  void createFramebuffers(Pipelines& _pipelines);
  void createDescriptorSetLayout();
  void recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex,
                                   Pipelines& _pipelines);
  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer,
                                  Pipelines& _pipelines);
  void updateUniformBuffer(uint32_t currentImage);
  void createImage(uint32_t width,
                   uint32_t height,
                   VkSampleCountFlagBits numSamples,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkImage& image,
                   VkDeviceMemory& imageMemory);
  VkImageView createImageView(VkImage image,
                              VkFormat format,
                              VkImageAspectFlags aspectFlags);

 private:
  VulkanMechanics& _mechanics;

  void createVertexBuffer();
  void createIndexBuffer();

  void createVertexBufferLandscape();
  void createIndexBufferLandscape();

  void setPushConstants();
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void createCommandBuffers();
  void createComputeCommandBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void createShaderStorageBuffers();
  void createUniformBuffers();
  void createBuffer(VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory);
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void copyBufferToImage(VkBuffer buffer,
                         VkImage image,
                         uint32_t width,
                         uint32_t height);
  void transitionImageLayout(VkImage image,
                             VkFormat format,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout);
  void createTextureImage(std::string imagePath);
  void createTextureSampler();
  void createTextureImageView();
};
