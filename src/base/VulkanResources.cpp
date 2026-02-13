#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanResources.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

#include "../core/Log.h"

#include <cstring>
#include <stdexcept>

uint32_t CE::findMemoryType(const uint32_t typeFilter,
                            const VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties{};
  vkGetPhysicalDeviceMemoryProperties(Device::baseDevice->physical,
                                      &memProperties);

  Log::text("{ MEM }", "Find Memory Type", "typeFilter", typeFilter);
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      Log::text(Log::Style::charLeader, "MemoryType index", i, "heap",
                memProperties.memoryTypes[i].heapIndex);
      return i;
    }
  }
  throw std::runtime_error("\n!ERROR! failed to find suitable memory type!");
}

CE::Buffer::~Buffer() {
  if (Device::baseDevice) {
    if (this->buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(Device::baseDevice->logical, this->buffer, nullptr);
      this->buffer = VK_NULL_HANDLE;
    }
    if (this->memory != VK_NULL_HANDLE) {
      vkFreeMemory(Device::baseDevice->logical, this->memory, nullptr);
      this->memory = VK_NULL_HANDLE;
    }
  }
}

void CE::Buffer::create(const VkDeviceSize& size,
                        const VkBufferUsageFlags& usage,
                        const VkMemoryPropertyFlags& properties,
                        Buffer& buffer) {
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .size = size,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  Log::text("{ ... }", Log::getBufferUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));
  Log::text(Log::Style::charLeader, size, "bytes");

  CE::VULKAN_RESULT(vkCreateBuffer, Device::baseDevice->logical, &bufferInfo,
                    nullptr, &buffer.buffer);

  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(Device::baseDevice->logical, buffer.buffer,
                                &memRequirements);
    Log::text("{ MEM }", "Buffer Memory Requirements");
    Log::text(Log::Style::charLeader, "requested", size, "aligned",
        memRequirements.size, "bytes");
    Log::text(Log::Style::charLeader, "alignment", memRequirements.alignment,
        "typeBits", memRequirements.memoryTypeBits);

    const uint32_t memoryTypeIndex =
      CE::findMemoryType(memRequirements.memoryTypeBits, properties);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = memoryTypeIndex};

    Log::text(Log::Style::charLeader, "alloc", allocateInfo.allocationSize,
        "bytes", "memoryTypeIndex", allocateInfo.memoryTypeIndex);

  CE::VULKAN_RESULT(vkAllocateMemory, Device::baseDevice->logical,
                    &allocateInfo, nullptr, &buffer.memory);
  vkBindBufferMemory(Device::baseDevice->logical, buffer.buffer, buffer.memory,
                     0);
}

void CE::Buffer::copy(const VkBuffer& srcBuffer,
                      VkBuffer& dstBuffer,
                      const VkDeviceSize size,
                      VkCommandBuffer& commandBuffer,
                      const VkCommandPool& commandPool,
                      const VkQueue& queue) {
  Log::text("{ ... }", "copying", size, "bytes");

  CE::CommandBuffers::beginSingularCommands(commandPool, queue);
  VkBufferCopy copyRegion{.size = size};
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  CE::CommandBuffers::endSingularCommands(commandPool, queue);
}

void CE::Buffer::copyToImage(const VkBuffer& buffer,
                             VkImage& image,
                             const uint32_t width,
                             const uint32_t height,
                             VkCommandBuffer& commandBuffer,
                             const VkCommandPool& commandPool,
                             const VkQueue& queue) {
  Log::text("{ img }", "Buffer To Image", width, height);

  CE::CommandBuffers::beginSingularCommands(commandPool, queue);
  VkBufferImageCopy region{.bufferOffset = 0,
                           .bufferRowLength = 0,
                           .bufferImageHeight = 0,
                           .imageOffset = {0, 0, 0},
                           .imageExtent = {width, height, 1}};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
  region.imageSubresource.mipLevel = 0,
  region.imageSubresource.baseArrayLayer = 0,
  region.imageSubresource.layerCount = 1,

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  CE::CommandBuffers::endSingularCommands(commandPool, queue);
}

void CE::Image::destroyVulkanImages() const {
  if (Device::baseDevice && this->memory) {
    if (this->sampler != VK_NULL_HANDLE) {
      vkDestroySampler(Device::baseDevice->logical, this->sampler, nullptr);
    };
    if (this->view != VK_NULL_HANDLE) {
      vkDestroyImageView(Device::baseDevice->logical, this->view, nullptr);
    };
    if (this->image != VK_NULL_HANDLE) {
      vkDestroyImage(Device::baseDevice->logical, this->image, nullptr);
    };
    if (this->memory != VK_NULL_HANDLE) {
      vkFreeMemory(Device::baseDevice->logical, this->memory, nullptr);
    };
  };
}

void CE::Image::create(const uint32_t width,
                       const uint32_t height,
                       const VkSampleCountFlagBits numSamples,
                       const VkFormat format,
                       const VkImageTiling tiling,
                       const VkImageUsageFlags& usage,
                       const VkMemoryPropertyFlags& properties) {
  Log::text("{ img }", "Image", width, height);
  Log::text(Log::Style::charLeader, Log::getSampleCountString(numSamples));
  Log::text(Log::Style::charLeader, Log::getImageUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));

  info.format = format;
  info.extent = {.width = width, .height = height, .depth = 1};
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = numSamples;
  info.tiling = tiling;
  info.usage = usage;

  CE::VULKAN_RESULT(vkCreateImage, Device::baseDevice->logical, &this->info,
                    nullptr, &this->image);

  VkMemoryRequirements memRequirements{};
  vkGetImageMemoryRequirements(Device::baseDevice->logical, this->image,
                               &memRequirements);
    Log::text("{ MEM }", "Image Memory Requirements");
    Log::text(Log::Style::charLeader, "extent", width, "x", height,
        "aligned", memRequirements.size, "bytes");
    Log::text(Log::Style::charLeader, "alignment", memRequirements.alignment,
        "typeBits", memRequirements.memoryTypeBits);

    const uint32_t memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = memoryTypeIndex};

    Log::text(Log::Style::charLeader, "alloc", allocateInfo.allocationSize,
        "bytes", "memoryTypeIndex", allocateInfo.memoryTypeIndex);

  CE::VULKAN_RESULT(vkAllocateMemory, Device::baseDevice->logical,
                    &allocateInfo, nullptr, &this->memory);
  vkBindImageMemory(Device::baseDevice->logical, this->image, this->memory, 0);
}

void CE::Image::createView(const VkImageAspectFlags aspectFlags) {
  Log::text(Log::Style::charLeader, "Image View");

  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = this->image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = this->info.format,
      .subresourceRange = {.aspectMask = aspectFlags,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  CE::VULKAN_RESULT(vkCreateImageView, Device::baseDevice->logical, &viewInfo,
                    nullptr, &this->view);
  return;
}

void CE::Image::transitionLayout(const VkCommandBuffer& commandBuffer,
                                 const VkFormat format,
                                 const VkImageLayout oldLayout,
                                 const VkImageLayout newLayout) {
  Log::text("{ SYNC }", "Image Layout Transition", oldLayout, "->",
            newLayout, "format", format);

  VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                               .oldLayout = oldLayout,
                               .newLayout = newLayout,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .image = this->image};
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage{};
  VkPipelineStageFlags destinationStage{};

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else {
    barrier.srcAccessMask =
        VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_MEMORY_READ_BIT |
        VK_ACCESS_MEMORY_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    destinationStage =
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }

  Log::text(Log::Style::charLeader, "srcAccess", barrier.srcAccessMask,
            "dstAccess", barrier.dstAccessMask);
  Log::text(Log::Style::charLeader, "srcStage", sourceStage, "dstStage",
            destinationStage);

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
}

void CE::Image::loadTexture(const std::string& imagePath,
                            const VkFormat format,
                            VkCommandBuffer& commandBuffer,
                            const VkCommandPool& commandPool,
                            const VkQueue& queue) {
  Log::text("{ img }", "Image Texture: ", imagePath);

  int texWidth(0), texHeight(0), texChannels(0), rgba(4);
  stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) *
                           static_cast<VkDeviceSize>(texHeight) *
                           static_cast<VkDeviceSize>(rgba);

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  Buffer stagingResources{};

  Buffer::create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingResources);

  void* data{};
  vkMapMemory(Device::baseDevice->logical, stagingResources.memory, 0,
              imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(Device::baseDevice->logical, stagingResources.memory);
  stbi_image_free(pixels);

  this->create(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CommandBuffers::beginSingularCommands(commandPool, queue);
  this->transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  CommandBuffers::endSingularCommands(commandPool, queue);

  Buffer::copyToImage(
      stagingResources.buffer, this->image, static_cast<uint32_t>(texWidth),
      static_cast<uint32_t>(texHeight), commandBuffer, commandPool, queue);

  CommandBuffers::beginSingularCommands(commandPool, queue);
  this->transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  CommandBuffers::endSingularCommands(commandPool, queue);
}

VkFormat CE::Image::findDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat CE::Image::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    const VkImageTiling tiling,
    const VkFormatFeatureFlags& features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(Device::baseDevice->physical, format,
                                        &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("\n!ERROR! failed to find supported format!");
}

void CE::Image::createResources(IMAGE_RESOURCE_TYPES imageType,
                                const VkExtent2D& dimensions,
                                const VkFormat format) {
  Log::text("{ []< }", "Color Resources ");
  this->destroyVulkanImages();

  VkImageUsageFlags usage = 0;
  VkImageAspectFlags aspect = 0;

  switch (imageType) {
    case CE_DEPTH_IMAGE:
      usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
      break;

    case CE_MULTISAMPLE_IMAGE:
      usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      aspect = VK_IMAGE_ASPECT_COLOR_BIT;
      break;

    default:
      Log::text("Unknown image type!", "Error");
      return;
  }

  this->create(dimensions.width, dimensions.height,
               Device::baseDevice->maxUsableSampleCount, format,
               VK_IMAGE_TILING_OPTIMAL, usage,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->createView(aspect);
}

void CE::Image::createSampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(Device::baseDevice->physical, &properties);

  VkSamplerCreateInfo samplerInfo{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE};

  if (vkCreateSampler(Device::baseDevice->logical, &samplerInfo, nullptr,
                      &this->sampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}
