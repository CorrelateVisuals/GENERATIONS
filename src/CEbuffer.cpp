#include "CEbuffer.h"

VkDevice* CEbuffer::_logicalDevice = nullptr;

CEbuffer::CEbuffer() : buffer{}, bufferMemory{}, mapped{} {}

CEbuffer::~CEbuffer() {
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

VkDevice* CEbuffer::setLogicalDevice(VkDevice* logicalDevice) {
  return logicalDevice;
};
