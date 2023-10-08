#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <stdexcept>

class CE {
 public:
  static VkDevice* _logicalDevice;
  static void linkLogicalDevice(VkDevice* logicalDevice);

  template <typename Checkresult, typename... Args>
  static void vulkanResult(Checkresult vkResult, Args&&... args);

  class Buffer {
   public:
    Buffer();
    virtual ~Buffer();

    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    void* mapped;
  };

  class Image {
   public:
    Image();
    virtual ~Image();

    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkSampler imageSampler;
    VkSampleCountFlagBits sampleCount;
  };

  class Descriptor {
   public:
    Descriptor();
    virtual ~Descriptor();

    VkDescriptorPool pool;
    VkDescriptorSetLayout setLayout;
    std::vector<VkDescriptorSet> sets;

    struct SetLayout {
      VkDescriptorSetLayoutBinding layoutBinding{
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM,
          .descriptorCount = 1,
          .stageFlags = NULL};
    };

   private:
    void createDescriptorPool();
    void allocateDescriptorSets();
    void createDescriptorSets();
  };
};

template <typename Checkresult, typename... Args>
void CE::vulkanResult(Checkresult vkResult,
    Args&&... args) {
    using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
    std::string objectName = typeid(ObjectType).name();

    VkResult result = vkResult(std::forward<Args>(args)...);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName +
            "!");
    }
}