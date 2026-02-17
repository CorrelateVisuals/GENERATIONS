#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Screenshot.h"

#include "core/Log.h"

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

void CE::Screenshot::capture(const VkImage &src_image,
                             const VkExtent2D &extent,
                             const VkFormat &format,
                             const VkCommandPool &command_pool,
                             const VkQueue &queue,
                             const std::string &filename) {
  Log::text("{ >>> }", "Screenshot:", filename);

  VkDeviceSize imageSize = static_cast<VkDeviceSize>(extent.width) *
                           static_cast<VkDeviceSize>(extent.height) *
                           static_cast<VkDeviceSize>(4);
  Buffer staging_buffer{};
  Buffer::create(imageSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging_buffer);

  copy_image_to_buffer(src_image, staging_buffer, extent, command_pool, queue);
  save_buffer_to_file(staging_buffer, extent, format, filename);

  Log::text(Log::Style::char_leader, "Screenshot queued for disk write");
}

void CE::Screenshot::copy_image_to_buffer(const VkImage &src_image,
                                          Buffer &dst_buffer,
                                       const VkExtent2D &extent,
                                       const VkCommandPool &command_pool,
                                       const VkQueue &queue) {
  CE::SingleUseCommands single_use_commands(command_pool, queue);
  VkCommandBuffer &command_buffer = single_use_commands.command_buffer();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = src_image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer,
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

  vkCmdCopyImageToBuffer(command_buffer,
                         src_image,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         dst_buffer.buffer,
                         1,
                         &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  vkCmdPipelineBarrier(command_buffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  single_use_commands.submit_and_wait();
}

void CE::Screenshot::save_buffer_to_file(const Buffer &buffer,
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

  const int width = static_cast<int>(extent.width);
  const int height = static_cast<int>(extent.height);
  const int stride = width * 4;

  if (!stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), stride)) {
    std::cerr << "Failed to write screenshot to file: " << filename << '\n';
  }
}
