#pragma once
#include <vulkan/vulkan.h>

class Buffer {
 public:
  Buffer();
  ~Buffer();

  VkBuffer buffer;
  VkDeviceMemory bufferMemory;
  void* mapped;

  static VkDevice* _logicalDevice;
  static VkDevice* setLogicalDevice(VkDevice* logicalDevice);
};

struct DescriptorSetLayout {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL
    };
};