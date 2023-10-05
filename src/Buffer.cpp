#include "Buffer.h"

VkDevice* Buffer::_logicalDevice = nullptr;

Buffer::Buffer() : buffer{}, bufferMemory{} {}

Buffer::~Buffer() {
  if (*_logicalDevice != VK_NULL_HANDLE) {
    if (buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(*_logicalDevice, buffer, nullptr);
      buffer = VK_NULL_HANDLE;
    }
    if (bufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(*_logicalDevice, bufferMemory, nullptr);
      bufferMemory = VK_NULL_HANDLE;
    }
  }
}
