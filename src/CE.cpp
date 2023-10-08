#include "CE.h"
#include "Log.h"

VkPhysicalDevice* CE::Device::physical = VK_NULL_HANDLE;
VkDevice* CE::Device::logical = VK_NULL_HANDLE;

VkCommandPool CE::CommandBuffer::commandPool = VK_NULL_HANDLE;
VkCommandBuffer CE::CommandBuffer::commandBuffer = VK_NULL_HANDLE;

void CE::Device::linkDevice(VkDevice* logicalDevice,
                            VkPhysicalDevice* physicalDevice) {
  Device::physical = physicalDevice;
  Device::logical = logicalDevice;
}
uint32_t CE::findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(*Device::physical, &memProperties);

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
    vkDestroyBuffer(*Device::logical, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (bufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(*Device::logical, bufferMemory, nullptr);
    bufferMemory = VK_NULL_HANDLE;
  }
}

void CE::Buffer::createBuffer(VkDeviceSize size,
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

  CE::vulkanResult(vkCreateBuffer, *Device::logical, &bufferInfo, nullptr,
                   &buffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(*Device::logical, buffer, &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          CE::findMemoryType(memRequirements.memoryTypeBits, properties)};

  CE::vulkanResult(vkAllocateMemory, *Device::logical, &allocateInfo,
                   nullptr, &bufferMemory);
  vkBindBufferMemory(*Device::logical, buffer, bufferMemory, 0);
}

CE::Image::Image()
    : image{},
      imageMemory{},
      imageView{},
      imageSampler{},
      sampleCount{VK_SAMPLE_COUNT_1_BIT} {}

CE::Image::~Image() {
  if (imageSampler != VK_NULL_HANDLE) {
    vkDestroySampler(*Device::logical, imageSampler, nullptr);
    imageSampler = VK_NULL_HANDLE;
  };
  if (imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(*Device::logical, imageView, nullptr);
    imageView = VK_NULL_HANDLE;
  };
  if (image != VK_NULL_HANDLE) {
    vkDestroyImage(*Device::logical, image, nullptr);
    image = VK_NULL_HANDLE;
  };
  if (imageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(*Device::logical, imageMemory, nullptr);
    imageMemory = VK_NULL_HANDLE;
  };
}

void CE::Image::createImage(uint32_t width,
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

  vulkanResult(vkCreateImage, *Device::logical, &imageInfo,
                   nullptr, &image);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(*Device::logical, image,
                               &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  vulkanResult(vkAllocateMemory, *Device::logical,
                   &allocateInfo, nullptr, &imageMemory);
  vkBindImageMemory(*Device::logical, image, imageMemory, 0);
}

CE::Descriptor::Descriptor() {
  createDescriptorPool();
  allocateDescriptorSets();
  createDescriptorSets();
}

CE::Descriptor::~Descriptor() {
  if (pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(*Device::logical, pool, nullptr);
  };
  if (setLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(*Device::logical, setLayout, nullptr);
  };
}

void CE::Descriptor::createDescriptorPool() {
  // std::vector<VkDescriptorPoolSize> poolSizes{
  //     {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  //      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
  //     {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
  //      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2},
  //     {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  //      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
  //     {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  //      .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)}};

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
}

void CE::Descriptor::allocateDescriptorSets() {
  // std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
  // setLayout); VkDescriptorSetAllocateInfo allocateInfo{
  //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
  //     .descriptorPool = pool,
  //     .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
  //     .pSetLayouts = layouts.data()};

  // sets.resize(MAX_FRAMES_IN_FLIGHT);
  //_mechanics.result(vkAllocateDescriptorSets, _mechanics.mainDevice.logical,
  //                   &allocateInfo, sets.data());
}

void CE::Descriptor::createDescriptorSets() {
  // Log::text("{ |=| }", "Descriptor Set Layout:", layoutBindings.size(),
  //           "bindings");
  // for (const VkDescriptorSetLayoutBinding& item : layoutBindings) {
  //   Log::text("{ ", item.binding, " }",
  //             Log::getDescriptorTypeString(item.descriptorType));
  //   Log::text(Log::Style::charLeader,
  //             Log::getShaderStageString(item.stageFlags));
  // }

  // VkDescriptorSetLayoutCreateInfo layoutInfo{
  //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
  //     .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
  //     .pBindings = layoutBindings.data()};

  //_mechanics.result(vkCreateDescriptorSetLayout,
  //_mechanics.mainDevice.logical,
  //                  &layoutInfo, nullptr, &setLayout);
}

void CE::CommandBuffer::createCommandPool(VkCommandPool* commandPool) {
  // Log::text("{ cmd }", "Command Pool");

  // VulkanMechanics::Queues::FamilyIndices queueFamilyIndices =
  //     findQueueFamilies(mainDevice.physical);

  // VkCommandPoolCreateInfo poolInfo{
  //     .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
  //     .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
  //     .queueFamilyIndex =
  //     queueFamilyIndices.graphicsAndComputeFamily.value() };

  // CE::vulkanResult(vkCreateCommandPool, *Device::logical, &poolInfo,
  //     nullptr, commandPool);
}

void CE::CommandBuffer::beginSingularCommands(VkCommandBuffer& commandBuffer) {
  // Log::text("{ 1.. }", "Begin Single Time Commands");

  // VkCommandBufferAllocateInfo allocInfo{
  //     .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
  //     .commandPool = command.pool,
  //     .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
  //     .commandBufferCount = 1};

  // vkAllocateCommandBuffers(*Device::logical, &allocInfo, &commandBuffer);

  // VkCommandBufferBeginInfo beginInfo{
  //     .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  //     .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  // vkBeginCommandBuffer(commandBuffer, &beginInfo);

  // return;
}

void CE::CommandBuffer::endSingluarCommands(VkCommandBuffer& commandBuffer) {}
