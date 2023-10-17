#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "BaseClasses.h"

#include <algorithm>
#include <set>

std::vector<VkDevice> CE::Device::destroyedDevices;
VkCommandBuffer CE::Commands::singularCommandBuffer = VK_NULL_HANDLE;

void CE::Device::createLogicalDevice(const InitializeVulkan& initVulkan,
                                     Queues& queues) {
  Log::text("{ +++ }", "Logical Device");
  // Queues::FamilyIndices indices =
  //     findQueueFamilies(mainDevice.physical, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      queues.familyIndices.graphicsAndComputeFamily.value(),
      queues.familyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
      .pEnabledFeatures = &features};

  if (initVulkan.validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(initVulkan.validation.validation.size());
    createInfo.ppEnabledLayerNames = initVulkan.validation.validation.data();
  }

  CE::vulkanResult(vkCreateDevice, physical, &createInfo, nullptr, &logical);

  vkGetDeviceQueue(logical,
                   queues.familyIndices.graphicsAndComputeFamily.value(), 0,
                   &queues.graphics);
  vkGetDeviceQueue(logical,
                   queues.familyIndices.graphicsAndComputeFamily.value(), 0,
                   &queues.compute);
  vkGetDeviceQueue(logical, queues.familyIndices.presentFamily.value(), 0,
                   &queues.present);
}

// template <typename SC>
void CE::Device::pickPhysicalDevice(const InitializeVulkan& initVulkan,
                                    Queues& queues,
                                    Swapchain& swapchain) {
  Log::text("{ ### }", "Physical Device");
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error(
        "\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device, queues, initVulkan, swapchain)) {
      physical = device;
      getMaxUsableSampleCount(physical);
      break;
    }
  }
  if (physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

// template <typename NSC>
bool CE::Device::isDeviceSuitable(const VkPhysicalDevice& physical,
                                  Queues& queues,
                                  const InitializeVulkan& initVulkan,
                                  Swapchain& swapchain) {
  Log::text(Log::Style::charLeader, "Is Device Suitable");

  queues.familyIndices = queues.findQueueFamilies(physical, initVulkan.surface);
  bool extensionsSupported = checkDeviceExtensionSupport(physical);

  bool swapchainAdequate = false;
  if (extensionsSupported) {
    Swapchain::SupportDetails swapchainSupport =
        swapchain.checkSupport(physical, initVulkan.surface);
    swapchainAdequate = !swapchainSupport.formats.empty() &&
                        !swapchainSupport.presentModes.empty();
  }
  // VkPhysicalDeviceFeatures supportedFeatures;
  // vkGetPhysicalDeviceFeatures(mainDevice.physical, &supportedFeatures);
  //&& supportedFeatures.samplerAnisotropy
  return queues.familyIndices.isComplete() && extensionsSupported &&
         swapchainAdequate;
}

void CE::Device::getMaxUsableSampleCount(const VkPhysicalDevice& physical) {
  vkGetPhysicalDeviceProperties(physical, &properties);
  VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts &
                              properties.limits.framebufferDepthSampleCounts;
  for (size_t i = VK_SAMPLE_COUNT_64_BIT; i >= VK_SAMPLE_COUNT_1_BIT; i >>= 1) {
    if (counts & i) {
      maxUsableSampleCount = static_cast<VkSampleCountFlagBits>(i);
      return;
    } else {
      maxUsableSampleCount = VK_SAMPLE_COUNT_1_BIT;
    }
  }
  return;
}

bool CE::Device::checkDeviceExtensionSupport(const VkPhysicalDevice& physical) {
  Log::text(Log::Style::charLeader, "Check Device Extension Support");
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(extensions.begin(),
                                           extensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

void CE::Device::setBaseDevice(const CE::Device& device) {
  baseDevice = std::make_unique<Device>(static_cast<const Device&>(device));
}

uint32_t CE::findMemoryType(const uint32_t typeFilter,
                            const VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(baseDevice->physical, &memProperties);

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
  if (baseDevice) {
    if (buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(baseDevice->logical, buffer, nullptr);
      buffer = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
      vkFreeMemory(baseDevice->logical, memory, nullptr);
      memory = VK_NULL_HANDLE;
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

  vulkanResult(vkCreateBuffer, baseDevice->logical, &bufferInfo, nullptr,
               &buffer.buffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(baseDevice->logical, buffer.buffer,
                                &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          CE::findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, baseDevice->logical, &allocateInfo, nullptr,
               &buffer.memory);
  vkBindBufferMemory(baseDevice->logical, buffer.buffer, buffer.memory, 0);
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
  if (baseDevice && memory) {
    if (sampler != VK_NULL_HANDLE) {
      vkDestroySampler(baseDevice->logical, sampler, nullptr);
      // sampler = VK_NULL_HANDLE;
    };
    if (view != VK_NULL_HANDLE) {
      vkDestroyImageView(baseDevice->logical, view, nullptr);
      // view = VK_NULL_HANDLE;
    };
    if (image != VK_NULL_HANDLE) {
      vkDestroyImage(baseDevice->logical, image, nullptr);
      // image = VK_NULL_HANDLE;
    };
    if (memory != VK_NULL_HANDLE) {
      vkFreeMemory(baseDevice->logical, memory, nullptr);
      // memory = VK_NULL_HANDLE;
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

  vulkanResult(vkCreateImage, baseDevice->logical, &info, nullptr, &image);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(baseDevice->logical, this->image,
                               &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, baseDevice->logical, &allocateInfo, nullptr,
               &this->memory);
  vkBindImageMemory(baseDevice->logical, this->image, this->memory, 0);
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

  vulkanResult(vkCreateImageView, baseDevice->logical, &viewInfo, nullptr,
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
  vkMapMemory(baseDevice->logical, stagingResources.memory, 0, imageSize, 0,
              &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(baseDevice->logical, stagingResources.memory);
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
    vkGetPhysicalDeviceFormatProperties(baseDevice->physical, format, &props);

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
                                     const VkFormat format) {
  Log::text("{ []< }", "Color Resources ");
  this->destroyVulkanImages();
  this->create(dimensions.width, dimensions.height,
               baseDevice->maxUsableSampleCount, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->createView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void CE::Image::createDepthResources(const VkExtent2D& dimensions,
                                     const VkFormat format) {
  Log::text("{ []< }", "Depth Resources ");
  this->destroyVulkanImages();
  this->create(dimensions.width, dimensions.height,
               baseDevice->maxUsableSampleCount, format,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  this->createView(VK_IMAGE_ASPECT_DEPTH_BIT);
}

void CE::Image::createSampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(baseDevice->physical, &properties);

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

  if (vkCreateSampler(baseDevice->logical, &samplerInfo, nullptr,
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
  if (baseDevice) {
    if (pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(baseDevice->logical, pool, nullptr);
    };
    if (setLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(baseDevice->logical, setLayout, nullptr);
    };
  }
}

CE::Commands::~Commands() {
  if (baseDevice && pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(baseDevice->logical, pool, nullptr);
  }
};

void CE::Commands::createCommandPool(
    const Queues::FamilyIndices& familyIndices) {
  Log::text("{ cmd }", "Command Pool");

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = familyIndices.graphicsAndComputeFamily.value()};

  CE::vulkanResult(vkCreateCommandPool, baseDevice->logical, &poolInfo, nullptr,
                   &pool);
}

void CE::Commands::beginSingularCommands(const VkCommandPool& commandPool,
                                         const VkQueue& queue) {
  Log::text("{ 1.. }", "Begin Single Time Commands");

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  vkAllocateCommandBuffers(baseDevice->logical, &allocInfo,
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
  vkFreeCommandBuffers(baseDevice->logical, commandPool, 1,
                       &singularCommandBuffer);
}

void CE::Device::destroyDevice() {
  static bool isDeviceDestroyed = false;
  for (const VkDevice& device : destroyedDevices) {
    if (device == logical) {
      isDeviceDestroyed = true;
      break;
    }
  }
  if (!isDeviceDestroyed) {
    Log::text("{ +++ }", "Destroy Device", logical, "@", &logical);
    extensions.clear();
    vkDestroyDevice(logical, nullptr);
    destroyedDevices.push_back(logical);
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

void CE::Swapchain::destroy() {
  if (baseDevice) {
    Log::text("{ <-> }", "Destroy Swapchain");

    for (size_t i = 0; i < framebuffers.size(); i++) {
      vkDestroyFramebuffer(baseDevice->logical, framebuffers[i], nullptr);
    }
    for (size_t i = 0; i < images.size(); i++) {
      vkDestroyImageView(baseDevice->logical, images[i].view, nullptr);
    }
    vkDestroySwapchainKHR(baseDevice->logical, swapchain, nullptr);
  }
}

void CE::Swapchain::recreate(const VkSurfaceKHR& surface,
                             const Queues& queues,
                             SynchronizationObjects& syncObjects) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(Window::get().window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Window::get().window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(baseDevice->logical);

  destroy();
  create(surface, queues);

  uint32_t reset = 1;
  syncObjects.currentFrame = reset;
}

void CE::Swapchain::create(const VkSurfaceKHR& surface, const Queues& queues) {
  Log::text("{ <-> }", "Swap Chain");
  Swapchain::SupportDetails swapchainSupport =
      checkSupport(baseDevice->physical, surface);
  VkSurfaceFormatKHR surfaceFormat =
      pickSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode = pickPresentMode(swapchainSupport.presentModes);
  VkExtent2D extent =
      pickExtent(Window::get().window, supportDetails.capabilities);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount;
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
      .preTransform = swapchainSupport.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE};

  // Queues::FamilyIndices indices = findQueueFamilies(device.physical,
  // surface);
  std::vector<uint32_t> queueFamilyIndices{
      queues.familyIndices.graphicsAndComputeFamily.value(),
      queues.familyIndices.presentFamily.value()};

  if (queues.familyIndices.graphicsAndComputeFamily !=
      queues.familyIndices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndices.size());
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  CE::vulkanResult(vkCreateSwapchainKHR, baseDevice->logical, &createInfo,
                   nullptr, &swapchain);

  vkGetSwapchainImagesKHR(baseDevice->logical, swapchain, &imageCount, nullptr);

  images.resize(imageCount);
  imageFormat = surfaceFormat.format;
  this->extent = extent;

  std::vector<VkImage> swapchainImages(2);
  vkGetSwapchainImagesKHR(baseDevice->logical, swapchain, &imageCount,
                          swapchainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    images[i].image = swapchainImages[i];
    images[i].info.format = imageFormat;
    images[i].createView(VK_IMAGE_ASPECT_COLOR_BIT);
  };
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

void CE::SynchronizationObjects::create(const int maxFramesInFlight) {
  Log::text("{ ||| }", "Sync Objects");

  imageAvailableSemaphores.resize(maxFramesInFlight);
  renderFinishedSemaphores.resize(maxFramesInFlight);
  computeFinishedSemaphores.resize(maxFramesInFlight);
  graphicsInFlightFences.resize(maxFramesInFlight);
  computeInFlightFences.resize(maxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    CE::vulkanResult(vkCreateSemaphore, baseDevice->logical, &semaphoreInfo,
                     nullptr, &imageAvailableSemaphores[i]);
    CE::vulkanResult(vkCreateSemaphore, baseDevice->logical, &semaphoreInfo,
                     nullptr, &renderFinishedSemaphores[i]);
    CE::vulkanResult(vkCreateFence, baseDevice->logical, &fenceInfo, nullptr,
                     &graphicsInFlightFences[i]);
    CE::vulkanResult(vkCreateSemaphore, baseDevice->logical, &semaphoreInfo,
                     nullptr, &computeFinishedSemaphores[i]);
    CE::vulkanResult(vkCreateFence, baseDevice->logical, &fenceInfo, nullptr,
                     &computeInFlightFences[i]);
  }
}

void CE::SynchronizationObjects::destroy(const int maxFramesInFlight) {
  if (baseDevice) {
    Log::text("{ ||| }", "Destroy Synchronization Objects");
    for (size_t i = 0; i < maxFramesInFlight; i++) {
      vkDestroySemaphore(baseDevice->logical, renderFinishedSemaphores[i],
                         nullptr);
      vkDestroySemaphore(baseDevice->logical, imageAvailableSemaphores[i],
                         nullptr);
      vkDestroySemaphore(baseDevice->logical, computeFinishedSemaphores[i],
                         nullptr);
      vkDestroyFence(baseDevice->logical, graphicsInFlightFences[i], nullptr);
      vkDestroyFence(baseDevice->logical, computeInFlightFences[i], nullptr);
    };
  }
}

CE::InitializeVulkan::InitializeVulkan() {
  Log::text("{ VkI }", "constructing Initialize Vulkan");
  createInstance();
  validation.setupDebugMessenger(instance);
  createSurface(Window::get().window);
}

CE::InitializeVulkan::~InitializeVulkan() {
  Log::text("{ VkI }", "destructing Initialize Vulkan");
  if (validation.enableValidationLayers) {
    validation.DestroyDebugUtilsMessengerEXT(
        instance, validation.debugMessenger, nullptr);
  }
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

void CE::InitializeVulkan::createInstance() {
  Log::text("{ VkI }", "Vulkan Instance");
  if (validation.enableValidationLayers &&
      !validation.checkValidationLayerSupport()) {
    throw std::runtime_error(
        "\n!ERROR! validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = Window::get().display.title,
                            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                            .pEngineName = "CAPITAL Engine",
                            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                            .apiVersion = VK_API_VERSION_1_3};
  Log::text(Log::Style::charLeader, appInfo.pApplicationName,
            appInfo.applicationVersion, "-", appInfo.pEngineName,
            appInfo.engineVersion, "-", "Vulkan", 1.3);

  auto extensions = getRequiredExtensions();

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validation.validation.size());
    createInfo.ppEnabledLayerNames = validation.validation.data();

    validation.populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }

  CE::vulkanResult(vkCreateInstance, &createInfo, nullptr, &instance);
}

void CE::InitializeVulkan::createSurface(GLFWwindow* window) {
  Log::text("{ [ ] }", "Surface");
  CE::vulkanResult(glfwCreateWindowSurface, instance, window, nullptr,
                   &surface);
}

std::vector<const char*> CE::InitializeVulkan::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);
  if (validation.enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  return extensions;
}

void CE::Pipeline::constructPipelinesFromShaders(
    std::unordered_map<std::string, CE::Pipeline>& pipelineObjects,
    const std::unordered_map<std::string, std::vector<std::string>> shaders) {
  for (const auto& entry : shaders) {
    CE::Pipeline newPipeline;
    newPipeline.name = entry.first;
    newPipeline.shaders = entry.second;
    pipelineObjects[entry.first] = newPipeline;
  }
};
