#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanBaseResources.h"
#include "VulkanBaseSync.h"
#include "VulkanBaseUtils.h"

#include "engine/Log.h"

#include <cstring>
#include <stdexcept>
#include <unordered_set>

uint32_t CE::find_memory_type(const uint32_t type_filter,
                              const VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties{};
  vkGetPhysicalDeviceMemoryProperties(BaseDevice::base_device->physical_device,
                                      &mem_properties);

  Log::text("{ MEM }",
            Log::function_name(__func__),
            "Find Memory Type",
            "typeFilter",
            type_filter);
  Log::text(Log::Style::char_leader, Log::get_memory_property_string(properties));

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      Log::text(Log::Style::char_leader,
                Log::function_name(__func__),
                "MemoryType index",
                i,
                "heap",
                mem_properties.memoryTypes[i].heapIndex);
      return i;
    }
  }
  throw std::runtime_error("\n!ERROR! failed to find suitable memory type!");
}

CE::BaseBuffer::~BaseBuffer() {
  if (BaseDevice::base_device) {
    if (this->buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(BaseDevice::base_device->logical_device, this->buffer, nullptr);
      this->buffer = VK_NULL_HANDLE;
    }
    if (this->memory != VK_NULL_HANDLE) {
      vkFreeMemory(BaseDevice::base_device->logical_device, this->memory, nullptr);
      this->memory = VK_NULL_HANDLE;
    }
  }
}

void CE::BaseBuffer::create(const VkDeviceSize &size,
                        const VkBufferUsageFlags &usage,
                        const VkMemoryPropertyFlags &properties,
                        BaseBuffer &buffer) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.flags = 0;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferInfo.queueFamilyIndexCount = 0;
  bufferInfo.pQueueFamilyIndices = nullptr;
  Log::text("{ ... }", Log::get_buffer_usage_string(usage));
  Log::text(Log::Style::char_leader, Log::get_memory_property_string(properties));
  Log::text(Log::Style::char_leader, size, "bytes");

  CE::vulkan_result(
      vkCreateBuffer,
      BaseDevice::base_device->logical_device,
      &bufferInfo,
      nullptr,
      &buffer.buffer);

  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(
      BaseDevice::base_device->logical_device, buffer.buffer, &memRequirements);
  Log::text("{ MEM }", Log::function_name(__func__), "BaseBuffer Memory Requirements");
  Log::text(Log::Style::char_leader,
            "requested",
            size,
            "aligned",
            memRequirements.size,
            "bytes");
  Log::text(Log::Style::char_leader,
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

  Log::text(Log::Style::char_leader,
            "alloc",
            allocateInfo.allocationSize,
            "bytes",
            "memoryTypeIndex",
            allocateInfo.memoryTypeIndex);

  CE::vulkan_result(vkAllocateMemory,
                    BaseDevice::base_device->logical_device,
                    &allocateInfo,
                    nullptr,
                    &buffer.memory);
  vkBindBufferMemory(
      BaseDevice::base_device->logical_device, buffer.buffer, buffer.memory, 0);
}

void CE::BaseBuffer::copy(const VkBuffer &src_buffer,
                      VkBuffer &dst_buffer,
                      const VkDeviceSize size,
                      VkCommandBuffer & /*command_buffer*/,
                      const VkCommandPool &command_pool,
                      const VkQueue &queue) {
  Log::text("{ ... }", "copying", size, "bytes");
  if (Log::gpu_trace_enabled()) {
    Log::text("{ XFR }",
              "BaseBuffer copy",
              "src",
              src_buffer,
              "dst",
              dst_buffer,
              "bytes",
              size,
              "pool",
              command_pool,
              "queue",
              queue);
  }

  CE::BaseSingleUseCommands single_use_commands(command_pool, queue);
  VkCommandBuffer &single_use_command_buffer = single_use_commands.command_buffer();
  VkBufferCopy copy_region{};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size = size;
  vkCmdCopyBuffer(single_use_command_buffer, src_buffer, dst_buffer, 1, &copy_region);
  single_use_commands.submit_and_wait();
}

void CE::BaseBuffer::copy_to_image(const VkBuffer &buffer,
                               VkImage &image,
                               const uint32_t width,
                               const uint32_t height,
                               VkCommandBuffer & /*command_buffer*/,
                               const VkCommandPool &command_pool,
                               const VkQueue &queue) {
  Log::text("{ img }", "BaseBuffer To BaseImage", width, height);
  if (Log::gpu_trace_enabled()) {
    Log::text("{ XFR }",
              "BaseBuffer->BaseImage",
              "src",
              buffer,
              "dst",
              image,
              "extent",
              width,
              "x",
              height,
              "pool",
              command_pool,
              "queue",
              queue);
  }

  CE::BaseSingleUseCommands single_use_commands(command_pool, queue);
  VkCommandBuffer &single_use_command_buffer = single_use_commands.command_buffer();
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

  vkCmdCopyBufferToImage(single_use_command_buffer,
                         buffer,
                         image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);
  single_use_commands.submit_and_wait();
}

void CE::BaseImage::destroy_vulkan_images() {
  if (BaseDevice::base_device && this->memory) {
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
      vkDestroySampler(BaseDevice::base_device->logical_device, this->sampler, nullptr);
      this->sampler = VK_NULL_HANDLE;
    };
    if (this->view != VK_NULL_HANDLE) {
      vkDestroyImageView(BaseDevice::base_device->logical_device, this->view, nullptr);
      this->view = VK_NULL_HANDLE;
    };
    if (this->image != VK_NULL_HANDLE) {
      vkDestroyImage(BaseDevice::base_device->logical_device, this->image, nullptr);
      this->image = VK_NULL_HANDLE;
    };
    if (this->memory != VK_NULL_HANDLE) {
      vkFreeMemory(BaseDevice::base_device->logical_device, this->memory, nullptr);
      this->memory = VK_NULL_HANDLE;
    };
  };
}

void CE::BaseImage::create(const uint32_t width,
                       const uint32_t height,
                       const VkSampleCountFlagBits num_samples,
                       const VkFormat format,
                       const VkImageTiling tiling,
                       const VkImageUsageFlags &usage,
                       const VkMemoryPropertyFlags &properties) {
  Log::text("{ img }", "BaseImage", width, height);
  Log::text(Log::Style::char_leader, Log::get_sample_count_string(num_samples));
  Log::text(Log::Style::char_leader, Log::get_image_usage_string(usage));
  Log::text(Log::Style::char_leader, Log::get_memory_property_string(properties));

  info.format = format;
  info.extent = {.width = width, .height = height, .depth = 1};
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = num_samples;
  info.tiling = tiling;
  info.usage = usage;

  CE::vulkan_result(
      vkCreateImage,
      BaseDevice::base_device->logical_device,
      &this->info,
      nullptr,
      &this->image);

  VkMemoryRequirements memRequirements{};
  vkGetImageMemoryRequirements(
      BaseDevice::base_device->logical_device, this->image, &memRequirements);
  Log::text("{ MEM }", Log::function_name(__func__), "BaseImage Memory Requirements");
  Log::text(Log::Style::char_leader,
            "extent",
            width,
            "x",
            height,
            "aligned",
            memRequirements.size,
            "bytes");
  Log::text(Log::Style::char_leader,
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

  Log::text(Log::Style::char_leader,
            "alloc",
            allocateInfo.allocationSize,
            "bytes",
            "memoryTypeIndex",
            allocateInfo.memoryTypeIndex);

  CE::vulkan_result(vkAllocateMemory,
                    BaseDevice::base_device->logical_device,
                    &allocateInfo,
                    nullptr,
                    &this->memory);
  vkBindImageMemory(BaseDevice::base_device->logical_device,
                    this->image,
                    this->memory,
                    0);
}

void CE::BaseImage::create_view(const VkImageAspectFlags aspect_flags) {
  Log::text(Log::Style::char_leader, "BaseImage View");

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.pNext = nullptr;
  viewInfo.flags = 0;
  viewInfo.image = this->image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = this->info.format;
  viewInfo.components = {};
  viewInfo.subresourceRange.aspectMask = aspect_flags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  CE::vulkan_result(
      vkCreateImageView,
      BaseDevice::base_device->logical_device,
      &viewInfo,
      nullptr,
      &this->view);
  return;
}

void CE::BaseImage::transition_layout(const VkCommandBuffer &command_buffer,
                                  const VkFormat format,
                       const VkImageLayout old_layout,
                       const VkImageLayout new_layout) {
  const std::string transition_key = std::to_string(static_cast<uint32_t>(format)) + ":" +
                         std::to_string(static_cast<uint32_t>(old_layout)) +
                                     "->" +
                         std::to_string(static_cast<uint32_t>(new_layout));
  static std::unordered_set<std::string> logged_transitions{};
  const bool should_log_transition = logged_transitions.insert(transition_key).second;

  if (should_log_transition) {
    Log::text("{ SYNC }",
              Log::function_name(__func__),
              "BaseImage Layout Transition",
              old_layout,
              "->",
              new_layout,
              "format",
              static_cast<uint32_t>(format));
  }

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = nullptr;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
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

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
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
    Log::text(Log::Style::char_leader,
              "srcAccess",
              barrier.srcAccessMask,
              "dstAccess",
              barrier.dstAccessMask);
    Log::text(
      Log::Style::char_leader, "srcStage", sourceStage, "dstStage", destinationStage);
  }

  vkCmdPipelineBarrier(command_buffer,
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

void CE::BaseImage::load_texture(const std::string &image_path,
                             const VkFormat format,
                             VkCommandBuffer &command_buffer,
                             const VkCommandPool &command_pool,
                             const VkQueue &queue) {
  Log::text("{ img }", "BaseImage Texture: ", image_path);

  int texWidth(0), texHeight(0), texChannels(0), rgba(4);
  stbi_uc *loaded_pixels =
      stbi_load(image_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  stbi_uc fallback_pixel[4] = {255, 255, 255, 255};
  const stbi_uc *pixels = loaded_pixels;

  if (!pixels) {
    Log::text("{ !!! }",
              "Texture load failed, using 1x1 fallback for",
              image_path,
              stbi_failure_reason());
    texWidth = 1;
    texHeight = 1;
    texChannels = rgba;
    pixels = fallback_pixel;
  }

  VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) *
                           static_cast<VkDeviceSize>(texHeight) *
                           static_cast<VkDeviceSize>(rgba);

  BaseBuffer stagingResources{};

  BaseBuffer::create(imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingResources);

  void *data{};
  if (Log::gpu_trace_enabled()) {
    Log::text("{ MAP }", "Map texture staging memory", stagingResources.memory, imageSize);
  }
  vkMapMemory(
      BaseDevice::base_device->logical_device,
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
  vkUnmapMemory(BaseDevice::base_device->logical_device, stagingResources.memory);
  if (loaded_pixels) {
    stbi_image_free(loaded_pixels);
  }

  this->create(texWidth,
               texHeight,
               VK_SAMPLE_COUNT_1_BIT,
               format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  {
    CE::BaseSingleUseCommands single_use_commands(command_pool, queue);
    this->transition_layout(single_use_commands.command_buffer(),
                          format,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    single_use_commands.submit_and_wait();
  }

  BaseBuffer::copy_to_image(stagingResources.buffer,
                        this->image,
                        static_cast<uint32_t>(texWidth),
                        static_cast<uint32_t>(texHeight),
                        command_buffer,
                        command_pool,
                        queue);

  {
    CE::BaseSingleUseCommands single_use_commands(command_pool, queue);
    this->transition_layout(single_use_commands.command_buffer(),
                          format,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    single_use_commands.submit_and_wait();
  }
}

VkFormat CE::BaseImage::find_depth_format() {
  return find_supported_format(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat CE::BaseImage::find_supported_format(const std::vector<VkFormat> &candidates,
                                          const VkImageTiling tiling,
                                          const VkFormatFeatureFlags &features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(BaseDevice::base_device->physical_device,
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

void CE::BaseImage::create_resources(IMAGE_RESOURCE_TYPES image_type,
                                 const VkExtent2D &dimensions,
                                 const VkFormat format) {
  Log::text("{ []< }", "Color VulkanResources ");
  this->destroy_vulkan_images();

  VkImageUsageFlags usage = 0;
  VkImageAspectFlags aspect = 0;

  switch (image_type) {
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
               BaseDevice::base_device->max_usable_sample_count,
               format,
               VK_IMAGE_TILING_OPTIMAL,
               usage,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->create_view(aspect);
}

void CE::BaseImage::create_sampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(BaseDevice::base_device->physical_device, &properties);

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
          BaseDevice::base_device->logical_device, &samplerInfo, nullptr, &this->sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}
