#pragma once

// BaseDescriptor-set allocation/update primitives.
// Exists to hide Vulkan descriptor boilerplate from higher-level resources.

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include <vulkan/vulkan.h>

namespace CE {

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr size_t NUM_DESCRIPTORS = 5;

class BaseDescriptorInterface {
public:
  size_t write_index{};
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets{};
  VkDescriptorSetLayout set_layout{};
  std::array<VkDescriptorSetLayoutBinding, NUM_DESCRIPTORS> set_layout_bindings{};
  std::array<std::array<VkWriteDescriptorSet, NUM_DESCRIPTORS>, MAX_FRAMES_IN_FLIGHT>
      descriptor_writes{};
  std::vector<VkDescriptorPoolSize> pool_sizes{};

  BaseDescriptorInterface() = default;
  BaseDescriptorInterface(const BaseDescriptorInterface &) = delete;
  BaseDescriptorInterface &operator=(const BaseDescriptorInterface &) = delete;
  BaseDescriptorInterface(BaseDescriptorInterface &&) = delete;
  BaseDescriptorInterface &operator=(BaseDescriptorInterface &&) = delete;
  virtual ~BaseDescriptorInterface();

  void initialize_sets();
  void update_sets();

private:
  VkDescriptorPool pool{};
  size_t active_descriptor_count_{};

  void create_set_layout();
  void create_pool();
  void allocate_sets();
};

class BaseDescriptor {
public:
  BaseDescriptor() = default;
  virtual ~BaseDescriptor(){};

protected:
  size_t my_index{0};
  VkDescriptorPoolSize pool_size{};
  VkDescriptorSetLayoutBinding set_layout_binding{};
  struct DescriptorInformation {
    std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> previous_frame{};
    std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> current_frame{};
  } info;
};

} // namespace CE

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = CE::MAX_FRAMES_IN_FLIGHT;
constexpr size_t NUM_DESCRIPTORS = CE::NUM_DESCRIPTORS;
