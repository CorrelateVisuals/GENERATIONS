#include "VulkanDescriptor.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include "engine/Log.h"

CE::DescriptorInterface::~DescriptorInterface() {
  if (CE::Device::base_device) {
    if (this->pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(
          CE::Device::base_device->logical_device, this->pool, nullptr);
      this->pool = VK_NULL_HANDLE;
    };
    if (this->set_layout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(
          CE::Device::base_device->logical_device, this->set_layout, nullptr);
      this->set_layout = VK_NULL_HANDLE;
    };
  }
}

void CE::DescriptorInterface::create_set_layout() {
  Log::text(
      "{ |=| }", "Descriptor Set Layout:", active_descriptor_count_, "bindings");
  for (size_t i = 0; i < active_descriptor_count_; ++i) {
    const VkDescriptorSetLayoutBinding &item = set_layout_bindings[i];
    Log::text(
      "{ ", item.binding, " }", Log::get_descriptor_type_string(item.descriptorType));
    Log::text(Log::Style::char_leader, Log::get_shader_stage_string(item.stageFlags));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(active_descriptor_count_),
      .pBindings = set_layout_bindings.data()};

  CE::vulkan_result(vkCreateDescriptorSetLayout,
                    CE::Device::base_device->logical_device,
                    &layoutInfo,
                    nullptr,
                    &this->set_layout);
}

void CE::DescriptorInterface::create_pool() {
  Log::text("{ |=| }", "Descriptor Pool");
  for (size_t i = 0; i < pool_sizes.size(); i++) {
    Log::text(Log::Style::char_leader, Log::get_descriptor_type_string(pool_sizes[i].type));
  }
  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
      .pPoolSizes = pool_sizes.data()};
  CE::vulkan_result(vkCreateDescriptorPool,
                    CE::Device::base_device->logical_device,
                    &poolInfo,
                    nullptr,
                    &this->pool);
}

void CE::DescriptorInterface::allocate_sets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, set_layout);
  VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts.data()};
  CE::vulkan_result(vkAllocateDescriptorSets,
                    CE::Device::base_device->logical_device,
                    &allocateInfo,
                    sets.data());
}

void CE::DescriptorInterface::initialize_sets() {
  active_descriptor_count_ = write_index;
  if (active_descriptor_count_ > NUM_DESCRIPTORS) {
    active_descriptor_count_ = NUM_DESCRIPTORS;
  }

  create_set_layout();
  create_pool();
  allocate_sets();
  update_sets();
}

void CE::DescriptorInterface::update_sets() {
  Log::text("{ |=| }", "Update Descriptor Sets");

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    for (size_t descriptor_index = 0; descriptor_index < active_descriptor_count_;
         ++descriptor_index) {
      auto &descriptor = descriptor_writes[i][descriptor_index];
      descriptor.dstSet = this->sets[i];
    }

    vkUpdateDescriptorSets(CE::Device::base_device->logical_device,
                           static_cast<uint32_t>(active_descriptor_count_),
                           descriptor_writes[i].data(),
                           0,
                           nullptr);
  }
}
