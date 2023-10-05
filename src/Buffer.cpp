#include "Buffer.h"
#include <iostream>
VkDevice* Buffer::_logicalDevice = nullptr;

Buffer::Buffer() : buffer{}, bufferMemory{} {}

Buffer::~Buffer() {
  if (*_logicalDevice != VK_NULL_HANDLE) {
    vkDestroyBuffer(*_logicalDevice, buffer, nullptr);
    vkFreeMemory(*_logicalDevice, bufferMemory, nullptr);
  }
}
