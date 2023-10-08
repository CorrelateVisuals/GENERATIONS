#include "CE.h"

VkDevice* CE::_logicalDevice = nullptr;

void CE::linkLogicalDevice(VkDevice* logicalDevice) {
  _logicalDevice = logicalDevice;
}

CE::Buffer::Buffer() : buffer{}, bufferMemory{}, mapped{} {}

CE::Buffer::~Buffer() {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(*_logicalDevice, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (bufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(*_logicalDevice, bufferMemory, nullptr);
    bufferMemory = VK_NULL_HANDLE;
  }
}

CE::Image::Image()
    : image{},
      imageMemory{},
      imageView{},
      imageSampler{},
      sampleCount{VK_SAMPLE_COUNT_1_BIT} {}

CE::Image::~Image() {
  if (imageSampler != VK_NULL_HANDLE) {
    vkDestroySampler(*_logicalDevice, imageSampler, nullptr);
    imageSampler = VK_NULL_HANDLE;
  };
  if (imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(*_logicalDevice, imageView, nullptr);
    imageView = VK_NULL_HANDLE;
  };
  if (image != VK_NULL_HANDLE) {
    vkDestroyImage(*_logicalDevice, image, nullptr);
    image = VK_NULL_HANDLE;
  };
  if (imageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(*_logicalDevice, imageMemory, nullptr);
    imageMemory = VK_NULL_HANDLE;
  };
}

CE::Descriptor::Descriptor() {
  createDescriptorPool();
  allocateDescriptorSets();
  createDescriptorSets();
}

CE::Descriptor::~Descriptor() {
  if (pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(*_logicalDevice, pool, nullptr);
  };
  if (setLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(*_logicalDevice, setLayout, nullptr);
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
