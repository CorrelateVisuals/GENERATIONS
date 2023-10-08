#pragma once
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
#include <vector>

class CE {
 public:
  struct Device {
    static VkPhysicalDevice* physical;
    static VkDevice* logical;

    static void linkDevice(VkDevice* logicalDevice,
                           VkPhysicalDevice* physicalDevice);
  };

  template <typename Checkresult, typename... Args>
  static void vulkanResult(Checkresult vkResult, Args&&... args);

  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties);

  struct CommandBuffer {
      static VkCommandPool commandPool;
      static VkCommandBuffer commandBuffer;

      static void createCommandPool(VkCommandPool* commandPool);
      static void beginSingularCommands(VkCommandBuffer& commandBuffer);
      static void endSingluarCommands(VkCommandBuffer& commandBuffer);
  };
 

  class Buffer {
   public:
    Buffer();
    virtual ~Buffer();

    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    void* mapped;

    static void createBuffer(VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties,
                             VkBuffer& buffer,
                             VkDeviceMemory& bufferMemory);
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
void CE::vulkanResult(Checkresult vkResult, Args&&... args) {
  using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
  std::string objectName = typeid(ObjectType).name();

  VkResult result = vkResult(std::forward<Args>(args)...);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName +
                             "!");
  }
}
