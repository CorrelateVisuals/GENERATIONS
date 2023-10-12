#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "CE.h"

#include <algorithm>

VkPhysicalDevice* CE::LinkedDevice::_physical = VK_NULL_HANDLE;
VkDevice* CE::LinkedDevice::_logical = VK_NULL_HANDLE;
VkCommandBuffer CE::Commands::singularCommandBuffer = VK_NULL_HANDLE;

void CE::LinkedDevice::linkDevice(VkDevice* logicalDevice,
                                  VkPhysicalDevice* physicalDevice) {
  LinkedDevice::_physical = physicalDevice;
  LinkedDevice::_logical = logicalDevice;
}

uint32_t CE::findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(*LinkedDevice::_physical, &memProperties);

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
    vkDestroyBuffer(*LinkedDevice::_logical, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(*LinkedDevice::_logical, memory, nullptr);
    memory = VK_NULL_HANDLE;
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

  vulkanResult(vkCreateBuffer, *LinkedDevice::_logical, &bufferInfo, nullptr,
               &buffer.buffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(*LinkedDevice::_logical, buffer.buffer,
                                &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          CE::findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, *LinkedDevice::_logical, &allocateInfo,
               nullptr, &buffer.memory);
  vkBindBufferMemory(*LinkedDevice::_logical, buffer.buffer, buffer.memory, 0);
}

void CE::Buffer::copy(const VkBuffer& srcBuffer,
                      VkBuffer& dstBuffer,
                      const VkDeviceSize size,
                      VkCommandBuffer& commandBuffer,
                      const VkCommandPool& commandPool,
                      const VkQueue& queue) {
  Log::text("{ ... }", "copying", size, "bytes");

  CE::Commands::beginSingularCommands(commandPool, queue);
  VkBufferCopy copyRegion{.size = size};
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  CE::Commands::endSingularCommands(commandPool, queue);
}

void CE::Buffer::copyToImage(const VkBuffer& buffer,
                             VkImage& image,
                             const uint32_t width,
                             const uint32_t height,
                             VkCommandBuffer& commandBuffer,
                             const VkCommandPool& commandPool,
                             const VkQueue& queue) {
  Log::text("{ img }", "Buffer To Image", width, height);

  CE::Commands::beginSingularCommands(commandPool, queue);
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
  CE::Commands::endSingularCommands(commandPool, queue);
}

CE::Image::Image() : image{}, memory{}, view{}, sampler{} {}

void CE::Image::destroyVulkanImages() {
  if (*LinkedDevice::_logical != VK_NULL_HANDLE) {
    if (sampler != VK_NULL_HANDLE) {
      vkDestroySampler(*LinkedDevice::_logical, sampler, nullptr);
      sampler = VK_NULL_HANDLE;
    };
    if (view != VK_NULL_HANDLE) {
      vkDestroyImageView(*LinkedDevice::_logical, view, nullptr);
      view = VK_NULL_HANDLE;
    };
    if (image != VK_NULL_HANDLE) {
      vkDestroyImage(*LinkedDevice::_logical, image, nullptr);
      image = VK_NULL_HANDLE;
    };
    if (memory != VK_NULL_HANDLE) {
      vkFreeMemory(*LinkedDevice::_logical, memory, nullptr);
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

  vulkanResult(vkCreateImage, *LinkedDevice::_logical, &info, nullptr, &image);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(*LinkedDevice::_logical, this->image,
                               &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, *LinkedDevice::_logical, &allocateInfo,
               nullptr, &this->memory);
  vkBindImageMemory(*LinkedDevice::_logical, this->image, this->memory, 0);
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

  vulkanResult(vkCreateImageView, *LinkedDevice::_logical, &viewInfo, nullptr,
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
                 stagingResources);

  void* data;
  vkMapMemory(*LinkedDevice::_logical, stagingResources.memory, 0, imageSize, 0,
              &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(*LinkedDevice::_logical, stagingResources.memory);
  stbi_image_free(pixels);

  this->create(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  Commands::beginSingularCommands(commandPool, queue);
  this->transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  Commands::endSingularCommands(commandPool, queue);

  Buffer::copyToImage(
      stagingResources.buffer, this->image, static_cast<uint32_t>(texWidth),
      static_cast<uint32_t>(texHeight), commandBuffer, commandPool, queue);

  Commands::beginSingularCommands(commandPool, queue);
  this->transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  Commands::endSingularCommands(commandPool, queue);
}

VkFormat CE::Image::findDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat CE::Image::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                        const VkImageTiling tiling,
                                        const VkFormatFeatureFlags& features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(*LinkedDevice::_physical, format,
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

void CE::Image::createColorResources(const VkExtent2D& dimensions,
                                     const VkFormat format,
                                     const VkSampleCountFlagBits samples) {
  Log::text("{ []< }", "Color Resources ");
  this->destroyVulkanImages();
  this->create(dimensions.width, dimensions.height, samples, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->createView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void CE::Image::createDepthResources(const VkExtent2D& dimensions,
                                     const VkFormat format,
                                     const VkSampleCountFlagBits samples) {
  Log::text("{ []< }", "Depth Resources ");
  this->destroyVulkanImages();
  this->create(dimensions.width, dimensions.height, samples, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->createView(VK_IMAGE_ASPECT_DEPTH_BIT);
}

void CE::Image::createSampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(*LinkedDevice::_physical, &properties);

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

  if (vkCreateSampler(*LinkedDevice::_logical, &samplerInfo, nullptr,
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
    vkDestroyDescriptorPool(*LinkedDevice::_logical, pool, nullptr);
  };
  if (setLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(*LinkedDevice::_logical, setLayout, nullptr);
  };
}

void CE::Commands::createCommandPool(
    const Queues::FamilyIndices& familyIndices) {
  Log::text("{ cmd }", "Command Pool");

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = familyIndices.graphicsAndComputeFamily.value()};

  CE::vulkanResult(vkCreateCommandPool, *LinkedDevice::_logical, &poolInfo,
                   nullptr, &pool);
}

void CE::Commands::beginSingularCommands(const VkCommandPool& commandPool,
                                         const VkQueue& queue) {
  Log::text("{ 1.. }", "Begin Single Time Commands");

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  vkAllocateCommandBuffers(*CE::LinkedDevice::_logical, &allocInfo,
                           &singularCommandBuffer);

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  vkBeginCommandBuffer(singularCommandBuffer, &beginInfo);

  return;
}

void CE::Commands::endSingularCommands(const VkCommandPool& commandPool,
                                       const VkQueue& queue) {
  Log::text("{ ..1 }", "End Single Time Commands");

  vkEndCommandBuffer(singularCommandBuffer);
  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .commandBufferCount = 1,
                          .pCommandBuffers = &singularCommandBuffer};

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(*CE::LinkedDevice::_logical, commandPool, 1,
                       &singularCommandBuffer);
}

void CE::Device::destroyDevice() {
  if (*LinkedDevice::_logical != VK_NULL_HANDLE) {
    Log::text("{ +++ }", "Destroy Device");
    vkDestroyDevice(this->logical, nullptr);
    this->logical = VK_NULL_HANDLE;
  }
}

CE::Swapchain::SupportDetails CE::Swapchain::checkSupport(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface) {
  Log::text(Log::Style::charLeader, "Query Swap Chain Support");
  {
    Swapchain::SupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         nullptr);
    if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physicalDevice, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, nullptr);
    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                &presentModeCount,
                                                details.presentModes.data());
    }

    supportDetails = details;
    return details;
  }
}

VkSurfaceFormatKHR CE::Swapchain::pickSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  Log::text(Log::Style::charLeader, "Choose Swap Surface Format");

  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR CE::Swapchain::pickPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  Log::text(Log::Style::charLeader, "Choose Swap Present Mode");
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_MAILBOX_KHR;
}

VkExtent2D CE::Swapchain::pickExtent(
    GLFWwindow* window,
    const VkSurfaceCapabilitiesKHR& capabilities) {
  Log::text(Log::Style::charLeader, "Choose Swap Extent");

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

CE::Queues::FamilyIndices CE::Queues::findQueueFamilies(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface) {
  Log::text(Log::Style::charLeader, "Find Queue Families");

  CE::Queues::FamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.graphicsAndComputeFamily = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }
    if (indices.isComplete()) {
      break;
    }
    i++;
  }
  return indices;
}
