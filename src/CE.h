#pragma once
#include <vulkan/vulkan.h>

#include <vector>

class CE {
 public:
  static VkDevice* _logicalDevice;
  static void setLogicalDevice(VkDevice* logicalDevice);

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
