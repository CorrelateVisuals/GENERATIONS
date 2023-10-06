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
    VkDescriptorSetLayoutBinding layoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL
    };
};