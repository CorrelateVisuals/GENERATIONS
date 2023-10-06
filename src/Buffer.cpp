#include "Buffer.h"

VkDevice* Buffer::_logicalDevice = nullptr;

Buffer::Buffer() : buffer{}, bufferMemory{}, mapped{} {}

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

VkDevice* Buffer::setLogicalDevice(VkDevice* logicalDevice) {
  return logicalDevice;
};
