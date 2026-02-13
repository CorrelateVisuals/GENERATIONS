#pragma once

#include <array>
#include <variant>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanCore.h"

namespace CE {

class DescriptorInterface {
public:
  size_t writeIndex{};
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets{};
  VkDescriptorSetLayout setLayout{};
  std::array<VkDescriptorSetLayoutBinding, NUM_DESCRIPTORS> setLayoutBindings{};
  std::array<std::array<VkWriteDescriptorSet, NUM_DESCRIPTORS>, MAX_FRAMES_IN_FLIGHT>
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

class Descriptor {
public:
  Descriptor() = default;
  virtual ~Descriptor(){};

protected:
  size_t myIndex{0};
  VkDescriptorPoolSize poolSize{};
  VkDescriptorSetLayoutBinding setLayoutBinding{};
  struct DescriptorInformation {
    std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> previousFrame{};
    std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> currentFrame{};
  } info;
};

} // namespace CE
