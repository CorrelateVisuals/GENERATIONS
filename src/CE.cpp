#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "CE.h"
#include "Log.h"

VkPhysicalDevice* CE::Device::_physical = VK_NULL_HANDLE;
VkDevice* CE::Device::_logical = VK_NULL_HANDLE;

// VkCommandPool CE::CommandBuffer::commandPool = VK_NULL_HANDLE;
// VkCommandBuffer CE::CommandBuffer::commandBuffer = VK_NULL_HANDLE;

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

CE::Buffer::Buffer() : buffer{}, bufferMemory{}, mapped{} {}

CE::Buffer::~Buffer() {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(*Device::_logical, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (bufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(*Device::_logical, bufferMemory, nullptr);
    bufferMemory = VK_NULL_HANDLE;
  }
}

void CE::Buffer::create(VkDeviceSize size,
                        VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkBuffer& buffer,
                        VkDeviceMemory& bufferMemory) {
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
               &bufferMemory);
  vkBindBufferMemory(*Device::_logical, buffer, bufferMemory, 0);
}

void CE::Buffer::copy(VkBuffer srcBuffer,
                      VkBuffer dstBuffer,
                      VkDeviceSize size,
                      VkCommandBuffer& commandBuffer,
                      VkCommandPool& commandPool,
                      VkQueue& queue) {
  Log::text("{ ... }", "copying", size, "bytes");

  CE::Commands::beginSingularCommands(commandBuffer, commandPool, queue);
  VkBufferCopy copyRegion{.size = size};
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  CE::Commands::endSingularCommands(commandBuffer, commandPool, queue);
}

void CE::Buffer::copyToImage(VkBuffer buffer,
                             VkImage image,
                             uint32_t width,
                             uint32_t height,
                             VkCommandBuffer& commandBuffer,
                             VkCommandPool& commandPool,
                             VkQueue& queue) {
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

CE::Image::Image()
    : image{},
      imageMemory{},
      imageView{},
      imageSampler{},
      sampleCount{VK_SAMPLE_COUNT_1_BIT} {}

CE::Image::~Image() {
  if (imageSampler != VK_NULL_HANDLE) {
    vkDestroySampler(*Device::_logical, imageSampler, nullptr);
    imageSampler = VK_NULL_HANDLE;
  };
  if (imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(*Device::_logical, imageView, nullptr);
    imageView = VK_NULL_HANDLE;
  };
  if (image != VK_NULL_HANDLE) {
    vkDestroyImage(*Device::_logical, image, nullptr);
    image = VK_NULL_HANDLE;
  };
  if (imageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(*Device::_logical, imageMemory, nullptr);
    imageMemory = VK_NULL_HANDLE;
  };
}

void CE::Image::create(uint32_t width,
                       uint32_t height,
                       VkSampleCountFlagBits numSamples,
                       VkFormat format,
                       VkImageTiling tiling,
                       VkImageUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkImage& image,
                       VkDeviceMemory& imageMemory) {
  Log::text("{ img }", "Image", width, height);
  Log::text(Log::Style::charLeader, Log::getSampleCountString(numSamples));
  Log::text(Log::Style::charLeader, Log::getImageUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));

  VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {.width = width, .height = height, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = numSamples,
      .tiling = tiling,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  vulkanResult(vkCreateImage, *Device::_logical, &imageInfo, nullptr, &image);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(*Device::_logical, image, &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, *Device::_logical, &allocateInfo, nullptr,
               &imageMemory);
  vkBindImageMemory(*Device::_logical, image, imageMemory, 0);
}

VkImageView CE::Image::createView(VkImage image,
                                  VkFormat format,
                                  VkImageAspectFlags aspectFlags) {
  Log::text("{ ... }", ":  Image View");

  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {.aspectMask = aspectFlags,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  VkImageView imageView;
  vulkanResult(vkCreateImageView, *Device::_logical, &viewInfo, nullptr,
               &imageView);

  return imageView;
}

void CE::Image::transitionLayout(VkCommandBuffer commandBuffer,
                                 VkImage image,
                                 VkFormat format,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                               .oldLayout = oldLayout,
                               .newLayout = newLayout,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .image = image};
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
                             CE::Image& image,
                             VkCommandBuffer& commandBuffer,
                             VkCommandPool& commandPool,
                             VkQueue& queue) {
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
                  stagingResources.buffer, stagingResources.bufferMemory);

   void* data;
   vkMapMemory(*Device::_logical, stagingResources.bufferMemory, 0, imageSize,
   0,
               &data);
   memcpy(data, pixels, static_cast<size_t>(imageSize));
   vkUnmapMemory(*Device::_logical, stagingResources.bufferMemory);
   stbi_image_free(pixels);

   Image::create(
       texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
       VK_IMAGE_TILING_OPTIMAL,
       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image.image, image.imageMemory);

   Commands::beginSingularCommands(commandBuffer, commandPool, queue);
   Image::transitionLayout(
       commandBuffer, image.image, VK_FORMAT_R8G8B8A8_SRGB,
       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
   Commands::endSingularCommands(commandBuffer, commandPool, queue);

   Buffer::copyToImage(
       stagingResources.buffer, image.image, static_cast<uint32_t>(texWidth),
       static_cast<uint32_t>(texHeight), commandBuffer, commandPool, queue);

   Commands::beginSingularCommands(commandBuffer, commandPool, queue);
   Image::transitionLayout(commandBuffer, image.image,
                                VK_FORMAT_R8G8B8A8_SRGB,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
   Commands::endSingularCommands(commandBuffer, commandPool, queue);
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
                                         VkCommandPool& commandPool,
                                         VkQueue& queue) {
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

void CE::Commands::endSingularCommands(VkCommandBuffer& commandBuffer,
                                       VkCommandPool& commandPool,
                                       VkQueue& queue) {
  Log::text("{ ..1 }", "End Single Time Commands");

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .commandBufferCount = 1,
                          .pCommandBuffers = &commandBuffer};

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(*CE::Device::_logical, commandPool, 1, &commandBuffer);
}
