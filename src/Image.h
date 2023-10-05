#pragma once
#include "Buffer.h"
#include "vulkan/vulkan.h"

class Image {
 public:
  Image();
  ~Image();

  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  VkSampler imageSampler;
  VkSampleCountFlagBits sampleCount;

  static VkDevice* _logicalDevice;
  static VkDevice* setLogicalDevice(VkDevice* logicalDevice) {
    return logicalDevice;
  };
};
