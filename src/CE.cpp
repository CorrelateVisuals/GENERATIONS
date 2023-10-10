#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "CE.h"
#include "Log.h"

VkPhysicalDevice* CE::Device::_physical = VK_NULL_HANDLE;
VkDevice* CE::Device::_logical = VK_NULL_HANDLE;

void CE::Device::linkDevice(VkDevice* logicalDevice,
                            VkPhysicalDevice* physicalDevice) {
  Device::_physical = physicalDevice;
  Device::_logical = logicalDevice;
}
uint32_t CE::findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(*Device::_physical, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("\n!ERROR! failed to find suitable memory type!");
}

CE::Buffer::Buffer() : buffer{}, memory{}, mapped{} {}

CE::Buffer::~Buffer() {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(*Device::_logical, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(*Device::_logical, memory, nullptr);
    memory = VK_NULL_HANDLE;
  }
}

void CE::Buffer::create(VkDeviceSize size,
                        VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkBuffer& buffer,
                        VkDeviceMemory& memory) {
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .size = size,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  Log::text("{ ... }", Log::getBufferUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));
  Log::text(Log::Style::charLeader, size, "bytes");

  vulkanResult(vkCreateBuffer, *Device::_logical, &bufferInfo, nullptr,
               &buffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(*Device::_logical, buffer, &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          CE::findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, *Device::_logical, &allocateInfo, nullptr,
               &memory);
  vkBindBufferMemory(*Device::_logical, buffer, memory, 0);
}

void CE::Buffer::copy(const VkBuffer& srcBuffer,
                      VkBuffer& dstBuffer,
                      const VkDeviceSize size,
                      VkCommandBuffer& commandBuffer,
                      const VkCommandPool& commandPool,
                      const VkQueue& queue) {
  Log::text("{ ... }", "copying", size, "bytes");

  CE::Commands::beginSingularCommands(commandBuffer, commandPool, queue);
  VkBufferCopy copyRegion{.size = size};
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  CE::Commands::endSingularCommands(commandBuffer, commandPool, queue);
}

void CE::Buffer::copyToImage(const VkBuffer buffer,
                             VkImage& image,
                             const uint32_t width,
                             const uint32_t height,
                             VkCommandBuffer& commandBuffer,
                             const VkCommandPool& commandPool,
                             const VkQueue& queue) {
  Log::text("{ >>> }", "Buffer To Image", width, height);

  CE::Commands::beginSingularCommands(commandBuffer, commandPool, queue);
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
  CE::Commands::endSingularCommands(commandBuffer, commandPool, queue);
}

CE::Image::Image() : image{}, memory{}, view{}, sampler{} {}

CE::Image::~Image() {
  destroyVulkanObjects();
}

void CE::Image::destroyVulkanObjects() {
  if (*Device::_logical != VK_NULL_HANDLE) {
    if (sampler != VK_NULL_HANDLE) {
      vkDestroySampler(*Device::_logical, sampler, nullptr);
      sampler = VK_NULL_HANDLE;
    };
    if (view != VK_NULL_HANDLE) {
      vkDestroyImageView(*Device::_logical, view, nullptr);
      view = VK_NULL_HANDLE;
    };
    if (image != VK_NULL_HANDLE) {
      vkDestroyImage(*Device::_logical, image, nullptr);
      image = VK_NULL_HANDLE;
    };
    if (memory != VK_NULL_HANDLE) {
      vkFreeMemory(*Device::_logical, memory, nullptr);
      memory = VK_NULL_HANDLE;
    };
  };
}

void CE::Image::create(const uint32_t width,
                       const uint32_t height,
                       const VkSampleCountFlagBits numSamples,
                       const VkFormat format,
                       const VkImageTiling tiling,
                       const VkImageUsageFlags& usage,
                       const VkMemoryPropertyFlags properties) {
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

  vulkanResult(vkCreateImage, *Device::_logical, &info, nullptr, &image);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(*Device::_logical, this->image,
                               &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, *Device::_logical, &allocateInfo, nullptr,
               &this->memory);
  vkBindImageMemory(*Device::_logical, this->image, this->memory, 0);
}

void CE::Image::createView(const VkImageAspectFlags aspectFlags) {
  Log::text("{ ... }", ":  Image View");

  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = this->image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = info.format,
      .subresourceRange = {.aspectMask = aspectFlags,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  vulkanResult(vkCreateImageView, *Device::_logical, &viewInfo, nullptr,
               &this->view);
  return;
}

void CE::Image::transitionLayout(const VkCommandBuffer& commandBuffer,
                                 const VkFormat format,
                                 const VkImageLayout oldLayout,
                                 const VkImageLayout newLayout) {
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

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

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
    // throw std::invalid_argument("unsupported layout transition!");

    barrier.srcAccessMask =
        VK_ACCESS_MEMORY_WRITE_BIT;  // Every write must have finished...
    barrier.dstAccessMask =
        VK_ACCESS_MEMORY_READ_BIT |
        VK_ACCESS_MEMORY_WRITE_BIT;  // ... before it is safe to read or write
    // (Image Layout Transitions perform
    // both, read AND write access.)

    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // All commands must have
    // finished...
    destinationStage =
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // ...before any command may
                                             // continue. (Very heavy
                                             // barrier.)
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
}

void CE::Image::loadTexture(const std::string& imagePath,
                            const VkFormat format,
                            VkCommandBuffer& commandBuffer,
                            const VkCommandPool& commandPool,
                            const VkQueue& queue) {
  Log::text("{ img }", "Image Texture: ", imagePath);

  int texWidth{0}, texHeight{0}, texChannels{0};
  int rgba = 4;
  stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) *
                           static_cast<VkDeviceSize>(texHeight) *
                           static_cast<VkDeviceSize>(rgba);

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  Buffer stagingResources;

  Buffer::create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingResources.buffer, stagingResources.memory);

  void* data;
  vkMapMemory(*Device::_logical, stagingResources.memory, 0, imageSize, 0,
              &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(*Device::_logical, stagingResources.memory);
  stbi_image_free(pixels);

  this->create(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  Commands::beginSingularCommands(commandBuffer, commandPool, queue);
  this->transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  Commands::endSingularCommands(commandBuffer, commandPool, queue);

  Buffer::copyToImage(
      stagingResources.buffer, this->image, static_cast<uint32_t>(texWidth),
      static_cast<uint32_t>(texHeight), commandBuffer, commandPool, queue);

  Commands::beginSingularCommands(commandBuffer, commandPool, queue);
  this->transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  Commands::endSingularCommands(commandBuffer, commandPool, queue);
}

VkFormat CE::Image::findDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat CE::Image::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling,
                                        VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(*Device::_physical, format, &props);

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

void CE::Image::createColorResources(const VkExtent2D& dimensions,
                                     const VkFormat format,
                                     const VkSampleCountFlagBits samples) {
  Log::text("{ []< }", "Color Resources ");
  recreate();
  create(dimensions.width, dimensions.height, samples, format,
         VK_IMAGE_TILING_OPTIMAL,
         VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  createView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void CE::Image::createDepthResources(const VkExtent2D& dimensions,
                                     const VkFormat format,
                                     const VkSampleCountFlagBits samples) {
  Log::text("{ []< }", "Depth Resources ");
  recreate();
  create(dimensions.width, dimensions.height, samples, format,
         VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  createView(VK_IMAGE_ASPECT_DEPTH_BIT);
}

void CE::Image::createSampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(*Device::_physical, &properties);

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

  if (vkCreateSampler(*Device::_logical, &samplerInfo, nullptr,
                      &this->sampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

CE::Descriptor::Descriptor() {
  // createDescriptorPool();
  // allocateDescriptorSets();
  // createDescriptorSets();
}

CE::Descriptor::~Descriptor() {
  if (pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(*Device::_logical, pool, nullptr);
  };
  if (setLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(*Device::_logical, setLayout, nullptr);
  };
}

// void CE::Descriptor::createDescriptorPool() {
//  std::vector<VkDescriptorPoolSize> poolSizes{
//      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
//      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2},
//      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
//      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
//       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)}};

// Log::text("{ |=| }", "Descriptor Pool");
// for (size_t i = 0; i < poolSizes.size(); i++) {
//   Log::text(Log::Style::charLeader,
//             Log::getDescriptorTypeString(poolSizes[i].type));
// }

// VkDescriptorPoolCreateInfo poolInfo{
//     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//     .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
//     .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
//     .pPoolSizes = poolSizes.data()};

// result(vkCreateDescriptorPool, _logicalDevice, &poolInfo, nullptr, &pool);
//}

// void CE::Descriptor::allocateDescriptorSets() {
//  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
//  setLayout); VkDescriptorSetAllocateInfo allocateInfo{
//      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
//      .descriptorPool = pool,
//      .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
//      .pSetLayouts = layouts.data()};

// sets.resize(MAX_FRAMES_IN_FLIGHT);
//_mechanics.result(vkAllocateDescriptorSets, _mechanics.mainDevice._logical,
//                   &allocateInfo, sets.data());
//}

// void CE::Descriptor::createDescriptorSets() {
//  Log::text("{ |=| }", "Descriptor Set Layout:", layoutBindings.size(),
//            "bindings");
//  for (const VkDescriptorSetLayoutBinding& item : layoutBindings) {
//    Log::text("{ ", item.binding, " }",
//              Log::getDescriptorTypeString(item.descriptorType));
//    Log::text(Log::Style::charLeader,
//              Log::getShaderStageString(item.stageFlags));
//  }

// VkDescriptorSetLayoutCreateInfo layoutInfo{
//     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//     .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
//     .pBindings = layoutBindings.data()};

//_mechanics.result(vkCreateDescriptorSetLayout,
//_mechanics.mainDevice._logical,
//                  &layoutInfo, nullptr, &setLayout);
//}

// void CE::CommandBuffer::createCommandPool(VkCommandPool* commandPool) {
//  Log::text("{ cmd }", "Command Pool");

// VulkanMechanics::Queues::FamilyIndices queueFamilyIndices =
//     findQueueFamilies(mainDevice._physical);

// VkCommandPoolCreateInfo poolInfo{
//     .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//     .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
//     .queueFamilyIndex =
//     queueFamilyIndices.graphicsAndComputeFamily.value() };

// vulkanResult(vkCreateCommandPool, *Device::_logical, &poolInfo,
//     nullptr, commandPool);
//}

void CE::Commands::beginSingularCommands(VkCommandBuffer& commandBuffer,
                                         const VkCommandPool& commandPool,
                                         const VkQueue& queue) {
  Log::text("{ 1.. }", "Begin Single Time Commands");

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  vkAllocateCommandBuffers(*CE::Device::_logical, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return;
}

void CE::Commands::endSingularCommands(const VkCommandBuffer& commandBuffer,
                                       const VkCommandPool& commandPool,
                                       const VkQueue& queue) {
  Log::text("{ ..1 }", "End Single Time Commands");

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .commandBufferCount = 1,
                          .pCommandBuffers = &commandBuffer};

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(*CE::Device::_logical, commandPool, 1, &commandBuffer);
}
