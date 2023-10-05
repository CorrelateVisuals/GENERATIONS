#include "Buffer.h"

Buffer::Buffer() : buffer{}, bufferMemory{} {}

Buffer::~Buffer() {
  if (*_logicalDevice) {
    vkDestroyBuffer(*_logicalDevice, buffer, nullptr);
    vkFreeMemory(*_logicalDevice, bufferMemory, nullptr);
  }
}
