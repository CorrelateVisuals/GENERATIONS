#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanResources.h"
#include "VulkanSync.h"
#include "VulkanUtils.h"

#include "../core/Log.h"

#include <cstring>
#include <stdexcept>
#include <unordered_set>

uint32_t CE::find_memory_type(const uint32_t typeFilter,
                              const VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties{};
  vkGetPhysicalDeviceMemoryProperties(Device::base_device->physical_device,
                                      &memProperties);

  Log::text("{ MEM }",
            Log::function_name(__func__),
            "Find Memory Type",
            "typeFilter",
            typeFilter);
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      Log::text(Log::Style::charLeader,
                Log::function_name(__func__),
                "MemoryType index",
                i,
                "heap",
                memProperties.memoryTypes[i].heapIndex);
      return i;
    }
  }
  throw std::runtime_error("\n!ERROR! failed to find suitable memory type!");
}

CE::Buffer::~Buffer() {
  if (Device::base_device) {
    if (this->buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(Device::base_device->logical_device, this->buffer, nullptr);
      this->buffer = VK_NULL_HANDLE;
    }
    if (this->memory != VK_NULL_HANDLE) {
      vkFreeMemory(Device::base_device->logical_device, this->memory, nullptr);
      this->memory = VK_NULL_HANDLE;
    }
  }
}

void CE::Buffer::create(const VkDeviceSize &size,
                        const VkBufferUsageFlags &usage,
                        const VkMemoryPropertyFlags &properties,
                        Buffer &buffer) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.flags = 0;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferInfo.queueFamilyIndexCount = 0;
  bufferInfo.pQueueFamilyIndices = nullptr;
  Log::text("{ ... }", Log::getBufferUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));
  Log::text(Log::Style::charLeader, size, "bytes");

  CE::vulkan_result(
      vkCreateBuffer,
      Device::base_device->logical_device,
      &bufferInfo,
      nullptr,
      &buffer.buffer);

  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(
      Device::base_device->logical_device, buffer.buffer, &memRequirements);
  Log::text("{ MEM }", Log::function_name(__func__), "Buffer Memory Requirements");
  Log::text(Log::Style::charLeader,
            "requested",
            size,
            "aligned",
            memRequirements.size,
            "bytes");
  Log::text(Log::Style::charLeader,
            "alignment",
            memRequirements.alignment,
            "typeBits",
            memRequirements.memoryTypeBits);

  const uint32_t memory_type_index =
      CE::find_memory_type(memRequirements.memoryTypeBits, properties);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.allocationSize = memRequirements.size;
  allocateInfo.memoryTypeIndex = memory_type_index;

  Log::text(Log::Style::charLeader,
            "alloc",
            allocateInfo.allocationSize,
            "bytes",
            "memoryTypeIndex",
            allocateInfo.memoryTypeIndex);

  CE::vulkan_result(vkAllocateMemory,
                    Device::base_device->logical_device,
                    &allocateInfo,
                    nullptr,
                    &buffer.memory);
  vkBindBufferMemory(
      Device::base_device->logical_device, buffer.buffer, buffer.memory, 0);
}

void CE::Buffer::copy(const VkBuffer &srcBuffer,
                      VkBuffer &dstBuffer,
                      const VkDeviceSize size,
                      VkCommandBuffer &commandBuffer,
                      const VkCommandPool &commandPool,
                      const VkQueue &queue) {
  Log::text("{ ... }", "copying", size, "bytes");
  if (Log::gpu_trace_enabled()) {
    Log::text("{ XFR }",
              "Buffer copy",
              "src",
              srcBuffer,
              "dst",
              dstBuffer,
              "bytes",
              size,
              "pool",
              commandPool,
              "queue",
              queue);
  }

  CE::CommandBuffers::begin_singular_commands(commandPool, queue);
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  CE::CommandBuffers::end_singular_commands(commandPool, queue);
}

void CE::Buffer::copy_to_image(const VkBuffer &buffer,
                               VkImage &image,
                               const uint32_t width,
                               const uint32_t height,
                               VkCommandBuffer &commandBuffer,
                               const VkCommandPool &commandPool,
                               const VkQueue &queue) {
  Log::text("{ img }", "Buffer To Image", width, height);
  if (Log::gpu_trace_enabled()) {
    Log::text("{ XFR }",
              "Buffer->Image",
              "src",
              buffer,
              "dst",
              image,
              "extent",
              width,
              "x",
              height,
              "pool",
              commandPool,
              "queue",
              queue);
  }

  CE::CommandBuffers::begin_singular_commands(commandPool, queue);
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(
      commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  CE::CommandBuffers::end_singular_commands(commandPool, queue);
}

void CE::Image::destroy_vulkan_images() const {
  if (Device::base_device && this->memory) {
    if (Log::gpu_trace_enabled()) {
      Log::text("{ DST }",
                "Destroy image resources",
                "image",
                this->image,
                "view",
                this->view,
                "sampler",
                this->sampler,
                "memory",
                this->memory);
    }
    if (this->sampler != VK_NULL_HANDLE) {
      vkDestroySampler(Device::base_device->logical_device, this->sampler, nullptr);
    };
    if (this->view != VK_NULL_HANDLE) {
      vkDestroyImageView(Device::base_device->logical_device, this->view, nullptr);
    };
    if (this->image != VK_NULL_HANDLE) {
      vkDestroyImage(Device::base_device->logical_device, this->image, nullptr);
    };
    if (this->memory != VK_NULL_HANDLE) {
      vkFreeMemory(Device::base_device->logical_device, this->memory, nullptr);
    };
  };
}

void CE::Image::create(const uint32_t width,
                       const uint32_t height,
                       const VkSampleCountFlagBits numSamples,
                       const VkFormat format,
                       const VkImageTiling tiling,
                       const VkImageUsageFlags &usage,
                       const VkMemoryPropertyFlags &properties) {
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

  CE::vulkan_result(
      vkCreateImage,
      Device::base_device->logical_device,
      &this->info,
      nullptr,
      &this->image);

  VkMemoryRequirements memRequirements{};
  vkGetImageMemoryRequirements(
      Device::base_device->logical_device, this->image, &memRequirements);
  Log::text("{ MEM }", Log::function_name(__func__), "Image Memory Requirements");
  Log::text(Log::Style::charLeader,
            "extent",
            width,
            "x",
            height,
            "aligned",
            memRequirements.size,
            "bytes");
  Log::text(Log::Style::charLeader,
            "alignment",
            memRequirements.alignment,
            "typeBits",
            memRequirements.memoryTypeBits);

  const uint32_t memory_type_index =
      find_memory_type(memRequirements.memoryTypeBits, properties);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.allocationSize = memRequirements.size;
  allocateInfo.memoryTypeIndex = memory_type_index;

  Log::text(Log::Style::charLeader,
            "alloc",
            allocateInfo.allocationSize,
            "bytes",
            "memoryTypeIndex",
            allocateInfo.memoryTypeIndex);

  CE::vulkan_result(vkAllocateMemory,
                    Device::base_device->logical_device,
                    &allocateInfo,
                    nullptr,
                    &this->memory);
  vkBindImageMemory(Device::base_device->logical_device,
                    this->image,
                    this->memory,
                    0);
}

void CE::Image::create_view(const VkImageAspectFlags aspectFlags) {
  Log::text(Log::Style::charLeader, "Image View");

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.pNext = nullptr;
  viewInfo.flags = 0;
  viewInfo.image = this->image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = this->info.format;
  viewInfo.components = {};
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  CE::vulkan_result(
      vkCreateImageView,
      Device::base_device->logical_device,
      &viewInfo,
      nullptr,
      &this->view);
  return;
}

void CE::Image::transition_layout(const VkCommandBuffer &commandBuffer,
                                  const VkFormat format,
                                  const VkImageLayout oldLayout,
                                  const VkImageLayout newLayout) {
  const std::string transition_key = std::to_string(static_cast<uint32_t>(format)) + ":" +
                                     std::to_string(static_cast<uint32_t>(oldLayout)) +
                                     "->" +
                                     std::to_string(static_cast<uint32_t>(newLayout));
  static std::unordered_set<std::string> logged_transitions{};
  const bool should_log_transition = logged_transitions.insert(transition_key).second;

  if (should_log_transition) {
    Log::text("{ SYNC }",
              Log::function_name(__func__),
              "Image Layout Transition",
              oldLayout,
              "->",
              newLayout,
              "format",
              static_cast<uint32_t>(format));
  }

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = nullptr;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = this->image;
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
    destinationStage =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else {
    barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }

  if (should_log_transition) {
    Log::text(Log::Style::charLeader,
              "srcAccess",
              barrier.srcAccessMask,
              "dstAccess",
              barrier.dstAccessMask);
    Log::text(
        Log::Style::charLeader, "srcStage", sourceStage, "dstStage", destinationStage);
  }

  vkCmdPipelineBarrier(commandBuffer,
                       sourceStage,
                       destinationStage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);
}

void CE::Image::load_texture(const std::string &imagePath,
                             const VkFormat format,
                             VkCommandBuffer &commandBuffer,
                             const VkCommandPool &commandPool,
                             const VkQueue &queue) {
  Log::text("{ img }", "Image Texture: ", imagePath);

  int texWidth(0), texHeight(0), texChannels(0), rgba(4);
  stbi_uc *pixels =
      stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) *
                           static_cast<VkDeviceSize>(texHeight) *
                           static_cast<VkDeviceSize>(rgba);

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  Buffer stagingResources{};

  Buffer::create(imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingResources);

  void *data{};
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Map texture staging memory", stagingResources.memory, imageSize);
  }
  vkMapMemory(
      Device::base_device->logical_device,
      stagingResources.memory,
      0,
      imageSize,
      0,
      &data);
  if (Log::gpu_trace_enabled()) {
    Log::text("{ WR }", "Write host->staging texture bytes", imageSize);
  }
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Unmap texture staging memory", stagingResources.memory);
  }
  vkUnmapMemory(Device::base_device->logical_device, stagingResources.memory);
  stbi_image_free(pixels);

  this->create(texWidth,
               texHeight,
               VK_SAMPLE_COUNT_1_BIT,
               format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CommandBuffers::begin_singular_commands(commandPool, queue);
  this->transition_layout(commandBuffer,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  CommandBuffers::end_singular_commands(commandPool, queue);

  Buffer::copy_to_image(stagingResources.buffer,
                        this->image,
                        static_cast<uint32_t>(texWidth),
                        static_cast<uint32_t>(texHeight),
                        commandBuffer,
                        commandPool,
                        queue);

  CommandBuffers::begin_singular_commands(commandPool, queue);
  this->transition_layout(commandBuffer,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  CommandBuffers::end_singular_commands(commandPool, queue);
}

VkFormat CE::Image::find_depth_format() {
  return find_supported_format(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat CE::Image::find_supported_format(const std::vector<VkFormat> &candidates,
                                          const VkImageTiling tiling,
                                          const VkFormatFeatureFlags &features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(Device::base_device->physical_device,
                      format,
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

void CE::Image::create_resources(IMAGE_RESOURCE_TYPES imageType,
                                 const VkExtent2D &dimensions,
                                 const VkFormat format) {
  Log::text("{ []< }", "Color Resources ");
  this->destroy_vulkan_images();

  VkImageUsageFlags usage = 0;
  VkImageAspectFlags aspect = 0;

  switch (imageType) {
    case CE_DEPTH_IMAGE:
      usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
      break;

    case CE_MULTISAMPLE_IMAGE:
      usage =
          VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      aspect = VK_IMAGE_ASPECT_COLOR_BIT;
      break;

    default:
      Log::text("Unknown image type!", "Error");
      return;
  }

  this->create(dimensions.width,
               dimensions.height,
               Device::base_device->max_usable_sample_count,
               format,
               VK_IMAGE_TILING_OPTIMAL,
               usage,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->create_view(aspect);
}

void CE::Image::create_sampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(Device::base_device->physical_device, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.pNext = nullptr;
  samplerInfo.flags = 0;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  if (vkCreateSampler(
          Device::base_device->logical_device, &samplerInfo, nullptr, &this->sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}
