#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <variant>
#include <vector>

namespace CE {

// Constants
constexpr size_t NUM_DESCRIPTORS = 5;
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

/**
 * @brief Descriptor set interface for shader resource binding
 * 
 * Manages descriptor sets, layouts, and writes for binding
 * resources (buffers, images) to shaders.
 */
class DescriptorInterface {
 public:
  size_t writeIndex{};
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets{};
  VkDescriptorSetLayout setLayout{};
  std::array<VkDescriptorSetLayoutBinding, NUM_DESCRIPTORS> setLayoutBindings{};
  std::array<std::array<VkWriteDescriptorSet, NUM_DESCRIPTORS>,
             MAX_FRAMES_IN_FLIGHT>
      descriptorWrites{};
  std::vector<VkDescriptorPoolSize> poolSizes{};

  DescriptorInterface() = default;
  virtual ~DescriptorInterface();

  void initialzeSets();
  void updateSets();

 private:
  VkDescriptorPool pool;

  void createSetLayout();
  void createPool();
  void allocateSets();
};

/**
 * @brief Base descriptor class for shader resources
 * 
 * Provides common functionality for uniform buffers, storage buffers,
 * image samplers, and storage images.
 */
class Descriptor {
 public:
  Descriptor() = default;
  virtual ~Descriptor() {}

 protected:
  size_t myIndex{0};
  VkDescriptorPoolSize poolSize{};
  VkDescriptorSetLayoutBinding setLayoutBinding{};
  
  struct DescriptorInformation {
    std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> previousFrame{};
    std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> currentFrame{};
  } info;
};

}  // namespace CE
