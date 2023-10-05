#pragma once
#include <vulkan/vulkan.h>

class Buffer {
 public:
  Buffer();
  ~Buffer();

  VkBuffer buffer;
  VkDeviceMemory bufferMemory;
  void* voidPointer;

  static VkDevice* _logicalDevice;
  static VkDevice* setLogicalDevice(VkDevice* logicalDevice) {
    return logicalDevice;
  };
};
