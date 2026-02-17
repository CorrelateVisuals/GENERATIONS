# Code Proposal — CE-AUDIT-001 (2026-02-17)

This file aggregates proposed code changes from the automated agent run.

## C++ Lead
### Proposed Changes for `BaseDescriptorInterface`:
```cpp
namespace CE {

BaseDescriptorInterface::~BaseDescriptorInterface() {
  if (CE::BaseDevice::base_device) {
    if (this->pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(
          CE::BaseDevice::base_device->logical_device, this->pool, nullptr);
      this->pool = VK_NULL_HANDLE;
    }
    if (this->set_layout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(
          CE::BaseDevice::base_device->logical_device, this->set_layout, nullptr);
      this->set_layout = VK_NULL_HANDLE;
    }
  }
}

void CE::BaseDescriptorInterface::create_set_layout() {
  if (active_descriptor_count_ > NUM_DESCRIPTORS) {
    Log::error("Active descriptor count exceeds NUM_DESCRIPTORS");
    return;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(active_descriptor_count_),
      .pBindings = set_layout_bindings.data()};

  VkResult result = vkCreateDescriptorSetLayout(
      CE::BaseDevice::base_device->logical_device, &layoutInfo, nullptr, &set_layout);
  if (result != VK_SUCCESS) {
    Log::error("Failed to create descriptor set layout: ", result);
  }
}

void CE::BaseDescriptorInterface::allocate_sets() {
  static std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, set_layout);
  VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts.data()};

  VkResult result = vkAllocateDescriptorSets(
      CE::BaseDevice::base_device->logical_device, &allocateInfo, sets.data());
  if (result != VK_SUCCESS) {
    Log::error("Failed to allocate descriptor sets: ", result);
  }
}

void CE::BaseDescriptorInterface::update_sets() {
  Log::debug("{ |=| }", "Update BaseDescriptor Sets");

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    for (size_t descriptor_index = 0; descriptor_index < active_descriptor_count_;
         ++descriptor_index) {
      auto &descriptor = descriptor_writes[i][descriptor_index];
      descriptor.dstSet = this->sets[i];
    }

    vkUpdateDescriptorSets(CE::BaseDevice::base_device->logical_device,
                           static_cast<uint32_t>(active_descriptor_count_),
                           descriptor_writes[i].data(),
                           0,
                           nullptr);
  }
}

} // namespace CE
```

### Summary of Changes:
1. Added null checks in the destructor to prevent undefined behavior.
2. Added validation for Vulkan API calls and error handling.
3. Introduced static allocation of `layouts` in `allocate_sets` to avoid repeated allocations.
4. Replaced verbose logging with conditional logging using `Log::debug` and `Log::error`.
5. Added a check in `create_set_layout` to ensure `active_descriptor_count_` does not exceed `NUM_DESCRIPTORS`.

These changes improve resource management, error handling, and performance while adhering to the shared guardrails. Further optimizations and refactoring will be addressed by the subsequent agents.
