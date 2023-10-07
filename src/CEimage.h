#pragma once
#include "CEbuffer.h"
#include "vulkan/vulkan.h"

class CEimage {
 public:
  CEimage();
  virtual ~CEimage();

  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  VkSampler imageSampler;
  VkSampleCountFlagBits sampleCount;

  static VkDevice* _logicalDevice;
  static VkDevice* setLogicalDevice(VkDevice* logicalDevice);
};
