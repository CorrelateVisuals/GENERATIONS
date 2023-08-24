#pragma once
#include "vulkan/vulkan.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

class Resources {
 public:
  Resources();
  ~Resources();

  struct PushConstants {
    VkShaderStageFlags shaderStage = {VK_SHADER_STAGE_COMPUTE_BIT};
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
  void createResources();

  void createFramebuffers();
  void createDescriptorSetLayout();

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

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