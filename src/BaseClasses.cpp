#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "BaseClasses.h"
#include "Library.h"

#include <algorithm>
#include <set>

CE::Device* CE::Device::baseDevice = nullptr;
std::vector<VkDevice> CE::Device::destroyedDevices;
VkCommandBuffer CE::CommandBuffers::singularCommandBuffer = VK_NULL_HANDLE;

VkDescriptorPool CE::Descriptor::pool;
VkDescriptorSetLayout CE::Descriptor::setLayout;
std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> CE::Descriptor::sets;
std::vector<VkDescriptorPoolSize> CE::Descriptor::poolSizes;
std::vector<VkDescriptorSetLayoutBinding> CE::Descriptor::setLayoutBindings;
std::vector<CE::Descriptor::DescriptorInfo> CE::Descriptor::descriptorInfos;

void CE::Device::createLogicalDevice(const InitializeVulkan& initVulkan,
                                     Queues& queues) {
  Log::text("{ +++ }", "Logical Device");

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> uniqueQueueFamilies = {
      queues.familyIndices.graphicsAndComputeFamily.value(),
      queues.familyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint_fast8_t queueFamily : uniqueQueueFamilies) {
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
      .enabledExtensionCount = static_cast<uint32_t>(this->extensions.size()),
      .ppEnabledExtensionNames = this->extensions.data(),
      .pEnabledFeatures = &this->features};

  if (initVulkan.validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(initVulkan.validation.validation.size());
    createInfo.ppEnabledLayerNames = initVulkan.validation.validation.data();
  }

  CE::VULKAN_RESULT(vkCreateDevice, this->physical, &createInfo, nullptr,
                    &this->logical);

  vkGetDeviceQueue(this->logical,
                   queues.familyIndices.graphicsAndComputeFamily.value(), 0,
                   &queues.graphics);
  vkGetDeviceQueue(this->logical,
                   queues.familyIndices.graphicsAndComputeFamily.value(), 0,
                   &queues.compute);
  vkGetDeviceQueue(this->logical, queues.familyIndices.presentFamily.value(), 0,
                   &queues.present);
}

void CE::Device::pickPhysicalDevice(const InitializeVulkan& initVulkan,
                                    Queues& queues,
                                    Swapchain& swapchain) {
  Log::text("{ ### }", "Physical Device");
  uint32_t deviceCount(0);
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error(
        "\n!ERROR! failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(initVulkan.instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device, queues, initVulkan, swapchain)) {
      this->physical = device;
      getMaxUsableSampleCount();
      Log::text(Log::Style::charLeader,
                Log::getSampleCountString(this->maxUsableSampleCount));
      break;
    }
  }
  if (this->physical == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! failed to find a suitable GPU!");
  }
}

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

void CE::Device::getMaxUsableSampleCount() {
  vkGetPhysicalDeviceProperties(this->physical, &this->properties);
  VkSampleCountFlags counts =
      this->properties.limits.framebufferColorSampleCounts &
      this->properties.limits.framebufferDepthSampleCounts;
  for (uint_fast8_t i = VK_SAMPLE_COUNT_64_BIT; i >= VK_SAMPLE_COUNT_1_BIT;
       i >>= 1) {
    if (counts & i) {
      this->maxUsableSampleCount = static_cast<VkSampleCountFlagBits>(i);
      return;
    } else {
      this->maxUsableSampleCount = VK_SAMPLE_COUNT_1_BIT;
    }
  }
  return;
}

bool CE::Device::checkDeviceExtensionSupport(const VkPhysicalDevice& physical) {
  Log::text(Log::Style::charLeader, "Check Device Extension Support");
  uint32_t extensionCount(0);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(this->extensions.begin(),
                                           this->extensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  return requiredExtensions.empty();
}

uint32_t CE::findMemoryType(const uint32_t typeFilter,
                            const VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties{};
  vkGetPhysicalDeviceMemoryProperties(Device::baseDevice->physical,
                                      &memProperties);

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

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          CE::findMemoryType(memRequirements.memoryTypeBits, properties)};

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

CE::Image::Image() : image{}, memory{}, view{}, sampler{} {}

void CE::Image::destroyVulkanImages() {
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

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

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

VkFormat CE::Image::findSupportedFormat(const std::vector<VkFormat>& candidates,
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

void CE::Image::createResources(const VkExtent2D& dimensions,
                                const VkFormat format,
                                const VkImageUsageFlags usage,
                                const VkImageAspectFlagBits aspect) {
  Log::text("{ []< }", "Color Resources ");
  this->destroyVulkanImages();
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

CE::Descriptor::~Descriptor() {
  if (Device::baseDevice) {
    if (this->pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(Device::baseDevice->logical, this->pool, nullptr);
      this->pool = VK_NULL_HANDLE;
    };
    if (this->setLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(Device::baseDevice->logical, this->setLayout,
                                   nullptr);
      this->setLayout = VK_NULL_HANDLE;
    };
  }
}

void CE::Descriptor::createSetLayout(
    const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings) {
  Log::text("{ |=| }", "Descriptor Set Layout:", layoutBindings.size(),
            "bindings");
  for (const VkDescriptorSetLayoutBinding& item : layoutBindings) {
    Log::text("{ ", item.binding, " }",
              Log::getDescriptorTypeString(item.descriptorType));
    Log::text(Log::Style::charLeader,
              Log::getShaderStageString(item.stageFlags));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
      .pBindings = layoutBindings.data()};

  CE::VULKAN_RESULT(vkCreateDescriptorSetLayout,
                    CE::Device::baseDevice->logical, &layoutInfo, nullptr,
                    &CE::Descriptor::setLayout);
}

void CE::Descriptor::createSets() {}

void CE::Descriptor::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, setLayout);
  VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts.data()};
  CE::VULKAN_RESULT(vkAllocateDescriptorSets, Device::baseDevice->logical,
                    &allocateInfo, sets.data());
}

void CE::Descriptor::createPool() {
  Log::text("{ |=| }", "Descriptor Pool");
  for (size_t i = 0; i < poolSizes.size(); i++) {
    Log::text(Log::Style::charLeader,
              Log::getDescriptorTypeString(poolSizes[i].type));
  }
  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()};
  CE::VULKAN_RESULT(vkCreateDescriptorPool, Device::baseDevice->logical,
                    &poolInfo, nullptr, &CE::Descriptor::pool);
}

CE::CommandBuffers::~CommandBuffers() {
  if (Device::baseDevice && this->pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(Device::baseDevice->logical, this->pool, nullptr);
  }
};

void CE::CommandBuffers::createPool(
    const Queues::FamilyIndices& familyIndices) {
  Log::text("{ cmd }", "Command Pool");

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = familyIndices.graphicsAndComputeFamily.value()};

  CE::VULKAN_RESULT(vkCreateCommandPool, Device::baseDevice->logical, &poolInfo,
                    nullptr, &this->pool);
}

void CE::CommandBuffers::beginSingularCommands(const VkCommandPool& commandPool,
                                               const VkQueue& queue) {
  Log::text("{ 1.. }", "Begin Single Time Commands");

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  vkAllocateCommandBuffers(Device::baseDevice->logical, &allocInfo,
                           &singularCommandBuffer);

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  vkBeginCommandBuffer(singularCommandBuffer, &beginInfo);

  return;
}

void CE::CommandBuffers::endSingularCommands(const VkCommandPool& commandPool,
                                             const VkQueue& queue) {
  Log::text("{ ..1 }", "End Single Time Commands");

  vkEndCommandBuffer(singularCommandBuffer);
  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .commandBufferCount = 1,
                          .pCommandBuffers = &singularCommandBuffer};

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(Device::baseDevice->logical, commandPool, 1,
                       &singularCommandBuffer);
}

void CE::CommandBuffers::createBuffers(
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>& commandBuffers) {
  Log::text("{ cmd }", "Command Buffers:", MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = this->pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())};

  CE::VULKAN_RESULT(vkAllocateCommandBuffers, CE::Device::baseDevice->logical,
                    &allocateInfo, commandBuffers.data());
}

void CE::Device::destroyDevice() {
  static bool isDeviceDestroyed = false;
  for (const VkDevice& device : destroyedDevices) {
    if (device == this->logical) {
      isDeviceDestroyed = true;
      break;
    }
  }
  if (!isDeviceDestroyed) {
    Log::text("{ +++ }", "Destroy Device", this->logical, "@", &this->logical);
    extensions.clear();
    vkDestroyDevice(this->logical, nullptr);
    destroyedDevices.push_back(this->logical);
  }
}

CE::Swapchain::SupportDetails CE::Swapchain::checkSupport(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface) {
  Log::text(Log::Style::charLeader, "Query Swap Chain Support");
  {
    Swapchain::SupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &details.capabilities);
    uint32_t formatCount(0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         nullptr);
    if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physicalDevice, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount(0);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, nullptr);
    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                &presentModeCount,
                                                details.presentModes.data());
    }
    this->supportDetails = details;
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
    int width(0), height(0);
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
  if (Device::baseDevice) {
    Log::text("{ <-> }", "Destroy Swapchain");

    for (uint_fast8_t i = 0; i < this->framebuffers.size(); i++) {
      vkDestroyFramebuffer(Device::baseDevice->logical, this->framebuffers[i],
                           nullptr);
    }
    for (uint_fast8_t i = 0; i < this->images.size(); i++) {
      vkDestroyImageView(Device::baseDevice->logical, this->images[i].view,
                         nullptr);
    }
    vkDestroySwapchainKHR(Device::baseDevice->logical, this->swapchain,
                          nullptr);
  }
}

void CE::Swapchain::recreate(const VkSurfaceKHR& surface,
                             const Queues& queues,
                             SynchronizationObjects& syncObjects) {
  int width(0), height(0);
  glfwGetFramebufferSize(Window::get().window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Window::get().window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(Device::baseDevice->logical);

  destroy();
  create(surface, queues);

  uint32_t reset = 1;
  syncObjects.currentFrame = reset;
}

void CE::Swapchain::create(const VkSurfaceKHR& surface, const Queues& queues) {
  Log::text("{ <-> }", "Swap Chain");
  Swapchain::SupportDetails swapchainSupport =
      checkSupport(Device::baseDevice->physical, surface);
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

  CE::VULKAN_RESULT(vkCreateSwapchainKHR, Device::baseDevice->logical,
                    &createInfo, nullptr, &this->swapchain);

  vkGetSwapchainImagesKHR(Device::baseDevice->logical, this->swapchain,
                          &imageCount, nullptr);

  this->imageFormat = surfaceFormat.format;
  this->extent = extent;

  std::vector<VkImage> swapchainImages(MAX_FRAMES_IN_FLIGHT);
  vkGetSwapchainImagesKHR(Device::baseDevice->logical, this->swapchain,
                          &imageCount, swapchainImages.data());

  for (uint_fast8_t i = 0; i < imageCount; i++) {
    this->images[i].image = swapchainImages[i];
    this->images[i].info.format = this->imageFormat;
    this->images[i].createView(VK_IMAGE_ASPECT_COLOR_BIT);
  };
}

CE::Queues::FamilyIndices CE::Queues::findQueueFamilies(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface) {
  Log::text(Log::Style::charLeader, "Find Queue Families");

  CE::Queues::FamilyIndices indices{};
  uint32_t queueFamilyCount(0);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int i(0);
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

void CE::SynchronizationObjects::create() {
  Log::text("{ ||| }", "Sync Objects");

  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CE::VULKAN_RESULT(vkCreateSemaphore, Device::baseDevice->logical,
                      &semaphoreInfo, nullptr,
                      &this->imageAvailableSemaphores[i]);
    CE::VULKAN_RESULT(vkCreateSemaphore, Device::baseDevice->logical,
                      &semaphoreInfo, nullptr,
                      &this->renderFinishedSemaphores[i]);
    CE::VULKAN_RESULT(vkCreateFence, Device::baseDevice->logical, &fenceInfo,
                      nullptr, &this->graphicsInFlightFences[i]);
    CE::VULKAN_RESULT(vkCreateSemaphore, Device::baseDevice->logical,
                      &semaphoreInfo, nullptr,
                      &this->computeFinishedSemaphores[i]);
    CE::VULKAN_RESULT(vkCreateFence, Device::baseDevice->logical, &fenceInfo,
                      nullptr, &this->computeInFlightFences[i]);
  }
}

void CE::SynchronizationObjects::destroy() {
  if (Device::baseDevice) {
    Log::text("{ ||| }", "Destroy Synchronization Objects");
    for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(Device::baseDevice->logical,
                         this->renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(Device::baseDevice->logical,
                         this->imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(Device::baseDevice->logical,
                         this->computeFinishedSemaphores[i], nullptr);
      vkDestroyFence(Device::baseDevice->logical,
                     this->graphicsInFlightFences[i], nullptr);
      vkDestroyFence(Device::baseDevice->logical,
                     this->computeInFlightFences[i], nullptr);
    };
  }
}

CE::InitializeVulkan::InitializeVulkan() {
  Log::text("{ VkI }", "constructing Initialize Vulkan");
  createInstance();
  this->validation.setupDebugMessenger(this->instance);
  createSurface(Window::get().window);
}

CE::InitializeVulkan::~InitializeVulkan() {
  Log::text("{ VkI }", "destructing Initialize Vulkan");
  if (this->validation.enableValidationLayers) {
    this->validation.DestroyDebugUtilsMessengerEXT(
        this->instance, this->validation.debugMessenger, nullptr);
  }
  vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
  vkDestroyInstance(this->instance, nullptr);
}

void CE::InitializeVulkan::createInstance() {
  Log::text("{ VkI }", "Vulkan Instance");
  if (this->validation.enableValidationLayers &&
      !this->validation.checkValidationLayerSupport()) {
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

  std::vector<const char*> extensions = getRequiredExtensions();

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (this->validation.enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(this->validation.validation.size());
    createInfo.ppEnabledLayerNames = this->validation.validation.data();

    this->validation.populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }
  CE::VULKAN_RESULT(vkCreateInstance, &createInfo, nullptr, &this->instance);
}

void CE::InitializeVulkan::createSurface(GLFWwindow* window) {
  Log::text("{ [ ] }", "Surface");
  CE::VULKAN_RESULT(glfwCreateWindowSurface, this->instance, window, nullptr,
                    &this->surface);
}

std::vector<const char*> CE::InitializeVulkan::getRequiredExtensions() {
  uint32_t glfwExtensionCount(0);
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);
  if (this->validation.enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  return extensions;
}

CE::RenderPass::~RenderPass() {
  Log::text("{ []< }", "destructing Render Pass");
  if (Device::baseDevice) {
    vkDestroyRenderPass(Device::baseDevice->logical, this->renderPass, nullptr);
  }
}

void CE::RenderPass::create(VkSampleCountFlagBits msaaImageSamples,
                            VkFormat swapchainImageFormat) {
  Log::text("{ []< }", "Render Pass");
  Log::text(Log::Style::charLeader,
            "colorAttachment, depthAttachment, colorAttachmentResolve");

  VkAttachmentDescription colorAttachment{
      .format = swapchainImageFormat,
      .samples = msaaImageSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription depthAttachment{
      .format = CE::Image::findDepthFormat(),
      .samples = msaaImageSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription colorAttachmentResolve{
      .format = swapchainImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

  VkAttachmentReference colorAttachmentRef{
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentReference depthAttachmentRef{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentReference colorAttachmentResolveRef{
      .attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
      .pResolveAttachments = &colorAttachmentResolveRef,
      .pDepthStencilAttachment = &depthAttachmentRef};

  VkSubpassDependency dependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

  std::vector<VkAttachmentDescription> attachments = {
      colorAttachment, depthAttachment, colorAttachmentResolve};

  VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  CE::VULKAN_RESULT(vkCreateRenderPass, Device::baseDevice->logical,
                    &renderPassInfo, nullptr, &this->renderPass);
}

void CE::RenderPass::createFramebuffers(CE::Swapchain& swapchain,
                                        const VkImageView& msaaView,
                                        const VkImageView& depthView) {
  Log::text("{ 101 }", "Frame Buffers:", swapchain.images.size());

  Log::text(Log::Style::charLeader,
            "attachments: msaaImage., depthImage, swapchain imageViews");
  for (uint_fast8_t i = 0; i < swapchain.images.size(); i++) {
    std::array<VkImageView, 3> attachments{msaaView, depthView,
                                           swapchain.images[i].view};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .layers = 1};

    CE::VULKAN_RESULT(vkCreateFramebuffer, CE::Device::baseDevice->logical,
                      &framebufferInfo, nullptr, &swapchain.framebuffers[i]);
  }
}

CE::PipelinesConfiguration::~PipelinesConfiguration() {
  if (Device::baseDevice) {
    Log::text("{ === }", "destructing", this->pipelineMap.size(),
              "Pipelines Configuration");
    for (auto& pipeline : this->pipelineMap) {
      VkPipeline& pipelineObject = getPipelineObjectByName(pipeline.first);
      vkDestroyPipeline(Device::baseDevice->logical, pipelineObject, nullptr);
    }
  }
}

void CE::PipelinesConfiguration::createPipelines(
    VkRenderPass& renderPass,
    const VkPipelineLayout& graphicsLayout,
    const VkPipelineLayout& computeLayout,
    VkSampleCountFlagBits& msaaSamples) {
  for (auto& entry : this->pipelineMap) {
    const std::string pipelineName = entry.first;

    std::vector<std::string> shaders = getPipelineShadersByName(pipelineName);
    bool isCompute =
        std::find(shaders.begin(), shaders.end(), "Comp") != shaders.end();

    if (!isCompute) [[likely]] {
      Log::text("{ === }", "Graphics Pipeline: ", entry.first);
      std::variant<Graphics, Compute>& variant =
          this->pipelineMap[pipelineName];

      std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

      VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
      for (uint_fast8_t i = 0; i < shaders.size(); i++) {
        if (shaders[i] == "Vert") {
          shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (shaders[i] == "Frag") {
          shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (shaders[i] == "Tesc") {
          shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        } else if (shaders[i] == "Tese") {
          shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        shaderStages.push_back(createShaderModules(
            shaderStage, entry.first + shaders[i] + ".spv"));
      }

      const auto& bindingDescription =
          std::get<CE::PipelinesConfiguration::Graphics>(variant)
              .vertexBindings;
      const auto& attributesDescription =
          std::get<CE::PipelinesConfiguration::Graphics>(variant)
              .vertexAttributes;
      uint32_t bindingsSize = static_cast<uint32_t>(bindingDescription.size());
      uint32_t attributeSize =
          static_cast<uint32_t>(attributesDescription.size());

      for (const auto& item : bindingDescription) {
        Log::text(Log::Style::charLeader, "binding:", item.binding,
                  item.inputRate ? "VK_VERTEX_INPUT_RATE_INSTANCE"
                                 : "VK_VERTEX_INPUT_RATE_VERTEX");
      }

      VkPipelineVertexInputStateCreateInfo vertexInput{
          CE::vertexInputStateDefault};
      vertexInput.vertexBindingDescriptionCount = bindingsSize;
      vertexInput.vertexAttributeDescriptionCount = attributeSize;
      vertexInput.pVertexBindingDescriptions = bindingDescription.data();
      vertexInput.pVertexAttributeDescriptions = attributesDescription.data();

      VkPipelineInputAssemblyStateCreateInfo inputAssembly{
          CE::inputAssemblyStateTriangleList};

      VkPipelineRasterizationStateCreateInfo rasterization{
          CE::rasterizationCullBackBit};

      VkPipelineMultisampleStateCreateInfo multisampling{
          CE::multisampleStateDefault};
      multisampling.rasterizationSamples = msaaSamples;
      VkPipelineDepthStencilStateCreateInfo depthStencil{
          CE::depthStencilStateDefault};

      static VkPipelineColorBlendAttachmentState colorBlendAttachment{
          CE::colorBlendAttachmentStateFalse};
      VkPipelineColorBlendStateCreateInfo colorBlend{
          CE::colorBlendStateDefault};
      colorBlend.pAttachments = &colorBlendAttachment;

      VkPipelineViewportStateCreateInfo viewport{CE::viewportStateDefault};
      VkPipelineDynamicStateCreateInfo dynamic{CE::dynamicStateDefault};

      VkGraphicsPipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
          .stageCount = static_cast<uint32_t>(shaderStages.size()),
          .pStages = shaderStages.data(),
          .pVertexInputState = &vertexInput,
          .pInputAssemblyState = &inputAssembly,
          .pViewportState = &viewport,
          .pRasterizationState = &rasterization,
          .pMultisampleState = &multisampling,
          .pDepthStencilState = &depthStencil,
          .pColorBlendState = &colorBlend,
          .pDynamicState = &dynamic,
          .layout = graphicsLayout,
          .renderPass = renderPass,
          .subpass = 0,
          .basePipelineHandle = VK_NULL_HANDLE};

      CE::VULKAN_RESULT(vkCreateGraphicsPipelines, Device::baseDevice->logical,
                        VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                        &getPipelineObjectByName(pipelineName));
      destroyShaderModules();
    } else if (isCompute) {
      Log::text("{ === }", "Compute  Pipeline: ", entry.first);

      VkPipelineShaderStageCreateInfo shaderStage{createShaderModules(
          VK_SHADER_STAGE_COMPUTE_BIT, entry.first + shaders[0] + ".spv")};

      VkComputePipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage = shaderStage,
          .layout = computeLayout};

      CE::VULKAN_RESULT(vkCreateComputePipelines, Device::baseDevice->logical,
                        VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                        &getPipelineObjectByName(pipelineName));
      destroyShaderModules();
    }
  }
}

std::vector<char> CE::PipelinesConfiguration::readShaderFile(
    const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("\n!ERROR! failed to open file!");
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

VkPipelineShaderStageCreateInfo CE::PipelinesConfiguration::createShaderModules(
    VkShaderStageFlagBits shaderStage,
    std::string shaderName) {
  Log::text(Log::Style::charLeader, "Shader Module", shaderName);

  std::string shaderPath = this->shaderDir + shaderName;
  auto shaderCode = readShaderFile(shaderPath);
  VkShaderModule shaderModule{VK_NULL_HANDLE};

  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shaderCode.size(),
      .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())};

  CE::VULKAN_RESULT(vkCreateShaderModule, Device::baseDevice->logical,
                    &createInfo, nullptr, &shaderModule);

  shaderModules.push_back(shaderModule);

  VkPipelineShaderStageCreateInfo shaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = shaderStage,
      .module = shaderModule,
      .pName = "main"};

  return shaderStageInfo;
}

void CE::PipelinesConfiguration::compileShaders() {
  Log::text("{ GLSL }", "Compile Shaders");
  std::string systemCommand = "";
  std::string shaderExtension = "";
  std::string pipelineName = "";

  for (const auto& entry : this->pipelineMap) {
    pipelineName = entry.first;
    std::vector<std::string> shaders = getPipelineShadersByName(pipelineName);
    for (const auto& shader : shaders) {
      shaderExtension = Lib::upperToLowerCase(shader);
      systemCommand =
          Lib::path(this->shaderDir + pipelineName + "." + shaderExtension +
                    " -o " + this->shaderDir + pipelineName + shader + ".spv");
      system(systemCommand.c_str());
    }
  }
}

VkPipeline& CE::PipelinesConfiguration::getPipelineObjectByName(
    const std::string& name) {
  std::variant<Graphics, Compute>& variant = this->pipelineMap[name];
  if (std::holds_alternative<Graphics>(variant)) {
    return std::get<Graphics>(variant).pipeline;
  } else {
    return std::get<Compute>(variant).pipeline;
  }
}

void CE::PipelinesConfiguration::destroyShaderModules() {
  for (uint_fast8_t i = 0; i < this->shaderModules.size(); i++) {
    vkDestroyShaderModule(Device::baseDevice->logical, this->shaderModules[i],
                          nullptr);
  }
  this->shaderModules.resize(0);
};

std::vector<std::string>& CE::PipelinesConfiguration::getPipelineShadersByName(
    const std::string& name) {
  std::variant<Graphics, Compute>& variant = this->pipelineMap[name];

  if (std::holds_alternative<Graphics>(variant)) {
    return std::get<Graphics>(variant).shaders;
  } else {
    return std::get<Compute>(variant).shaders;
  }
}

const std::array<uint32_t, 3>& CE::PipelinesConfiguration::getWorkGroupsByName(
    const std::string& name) {
  std::variant<Graphics, Compute>& variant = this->pipelineMap[name];
  return std::get<CE::PipelinesConfiguration::Compute>(variant).workGroups;
};

void CE::PipelineLayout::createLayout(const VkDescriptorSetLayout& setLayout) {
  VkPipelineLayoutCreateInfo layout{CE::layoutDefault};
  layout.pSetLayouts = &setLayout;
  CE::VULKAN_RESULT(vkCreatePipelineLayout, Device::baseDevice->logical,
                    &layout, nullptr, &this->layout);
}

void CE::PipelineLayout::createLayout(const VkDescriptorSetLayout& setLayout,
                                      const PushConstants& _pushConstants) {
  VkPushConstantRange constants{.stageFlags = _pushConstants.shaderStage,
                                .offset = _pushConstants.offset,
                                .size = _pushConstants.size};
  VkPipelineLayoutCreateInfo layout{CE::layoutDefault};
  layout.pSetLayouts = &setLayout;
  layout.pushConstantRangeCount = _pushConstants.count;
  layout.pPushConstantRanges = &constants;
  CE::VULKAN_RESULT(vkCreatePipelineLayout, Device::baseDevice->logical,
                    &layout, nullptr, &this->layout);
}

CE::PipelineLayout::~PipelineLayout() {
  if (Device::baseDevice) {
    vkDestroyPipelineLayout(Device::baseDevice->logical, this->layout, nullptr);
  }
}

void CE::PushConstants::setData(const uint64_t& data) {
  this->data = {data};
}
