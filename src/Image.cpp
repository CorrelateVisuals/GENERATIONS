#include "Image.h"

#include <iostream>

VkDevice* Image::_logicalDevice = nullptr;

Image::Image()
    : image{},
      imageMemory{},
      imageView{},
      imageSampler{},
      sampleCount{VK_SAMPLE_COUNT_1_BIT} {}

Image::~Image() {
  if (*_logicalDevice != VK_NULL_HANDLE) {
    if (imageSampler != VK_NULL_HANDLE) {
      vkDestroySampler(*_logicalDevice, imageSampler, nullptr);
      imageSampler = VK_NULL_HANDLE;
    };
    if (imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(*_logicalDevice, imageView, nullptr);
      imageView = VK_NULL_HANDLE;
    };
    if (image != VK_NULL_HANDLE) {
      vkDestroyImage(*_logicalDevice, image, nullptr);
      image = VK_NULL_HANDLE;
    };
    if (imageMemory != VK_NULL_HANDLE) {
      vkFreeMemory(*_logicalDevice, imageMemory, nullptr);
      imageMemory = VK_NULL_HANDLE;
    };
  }
}

VkDevice* Image::setLogicalDevice(VkDevice* logicalDevice){
    return logicalDevice;
};
