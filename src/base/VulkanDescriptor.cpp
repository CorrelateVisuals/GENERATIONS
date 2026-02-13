#include "../BaseClasses.h"

CE::DescriptorInterface::~DescriptorInterface() {
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

void CE::DescriptorInterface::createSetLayout() {
  Log::text("{ |=| }", "Descriptor Set Layout:", setLayoutBindings.size(),
            "bindings");
  for (const VkDescriptorSetLayoutBinding& item : setLayoutBindings) {
    Log::text("{ ", item.binding, " }",
              Log::getDescriptorTypeString(item.descriptorType));
    Log::text(Log::Style::charLeader,
              Log::getShaderStageString(item.stageFlags));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(setLayoutBindings.size()),
      .pBindings = setLayoutBindings.data()};

  CE::VULKAN_RESULT(vkCreateDescriptorSetLayout,
                    CE::Device::baseDevice->logical, &layoutInfo, nullptr,
                    &this->setLayout);
}

void CE::DescriptorInterface::createPool() {
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
                    &poolInfo, nullptr, &this->pool);
}

void CE::DescriptorInterface::allocateSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, setLayout);
  VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts.data()};
  CE::VULKAN_RESULT(vkAllocateDescriptorSets, Device::baseDevice->logical,
                    &allocateInfo, sets.data());
}

void CE::DescriptorInterface::initialzeSets() {
  createSetLayout();
  createPool();
  allocateSets();
  updateSets();
}

void CE::DescriptorInterface::updateSets() {
  Log::text("{ |=| }", "Update Descriptor Sets");

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    for (auto& descriptor : descriptorWrites[i]) {
      descriptor.dstSet = this->sets[i];
    }

    vkUpdateDescriptorSets(CE::Device::baseDevice->logical,
                           static_cast<uint32_t>(descriptorWrites[i].size()),
                           descriptorWrites[i].data(), 0, nullptr);
  }
}