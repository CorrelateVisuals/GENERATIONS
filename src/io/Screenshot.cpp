#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Screenshot.h"

#include "core/Log.h"

#include <algorithm>
#include <cstring>
#include <vector>

void CE::Screenshot::capture(const VkImage &srcImage,
                             const VkExtent2D &extent,
                             const VkFormat &format,
                             const VkCommandPool &commandPool,
                             const VkQueue &queue,
                             const std::string &filename) {
  Log::text("{ >>> }", "Screenshot:", filename);

  VkDeviceSize imageSize = static_cast<VkDeviceSize>(extent.width) *
                           static_cast<VkDeviceSize>(extent.height) *
                           static_cast<VkDeviceSize>(4);
  Buffer stagingBuffer{};
  Buffer::create(imageSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer);

  copyImageToBuffer(srcImage, stagingBuffer, extent, commandPool, queue);
  saveBufferToFile(stagingBuffer, extent, format, filename);

  Log::text(Log::Style::charLeader, "Screenshot saved successfully");
}

void CE::Screenshot::copyImageToBuffer(const VkImage &srcImage,
                                       Buffer &dstBuffer,
                                       const VkExtent2D &extent,
                                       const VkCommandPool &commandPool,
                                       const VkQueue &queue) {
  CommandBuffers::begin_singular_commands(commandPool, queue);
  VkCommandBuffer &commandBuffer = CommandBuffers::singular_command_buffer;

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = srcImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {extent.width, extent.height, 1};

  vkCmdCopyImageToBuffer(commandBuffer,
                         srcImage,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         dstBuffer.buffer,
                         1,
                         &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  CommandBuffers::end_singular_commands(commandPool, queue);
}

void CE::Screenshot::saveBufferToFile(const Buffer &buffer,
                                      const VkExtent2D &extent,
                                      const VkFormat &format,
                                      const std::string &filename) {
  void *data{};
  vkMapMemory(
      Device::base_device->logical_device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &data);

  std::vector<uint8_t> pixels(extent.width * extent.height * 4);
  memcpy(pixels.data(), data, pixels.size());

  if (format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB ||
      format == VK_FORMAT_B8G8R8A8_SNORM) {
    for (size_t i = 0; i < pixels.size(); i += 4) {
      std::swap(pixels[i], pixels[i + 2]);
    }
  }

  vkUnmapMemory(Device::base_device->logical_device, buffer.memory);

  if (!stbi_write_png(filename.c_str(),
                      static_cast<int>(extent.width),
                      static_cast<int>(extent.height),
                      4,
                      pixels.data(),
                      static_cast<int>(extent.width) * 4)) {
    throw std::runtime_error("Failed to write screenshot to file: " + filename);
  }
}
