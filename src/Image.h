#pragma once
#include "vulkan/vulkan.h"

struct Image {
  Image(VkDevice& logicalDevice)
      : _logicalDevice(logicalDevice),
        image{},
        imageMemory{},
        imageView{},
        imageSampler{},
        samples{VK_SAMPLE_COUNT_1_BIT} {};

  VkDevice& _logicalDevice;

  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  VkSampler imageSampler;
  VkSampleCountFlagBits samples;

  ~Image() {
    vkDestroySampler(_logicalDevice, imageSampler, nullptr);
    vkDestroyImageView(_logicalDevice, imageView, nullptr);
    vkDestroyImage(_logicalDevice, image, nullptr);
    vkFreeMemory(_logicalDevice, imageMemory, nullptr);
  };
};
