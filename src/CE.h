#pragma once
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
#include <vector>

class CE {
 public:
  struct Device {
    static VkPhysicalDevice* _physical;
    static VkDevice* _logical;

    static void linkDevice(VkDevice* logicalDevice,
                           VkPhysicalDevice* physicalDevice);
  };

  class Commands {
   public:
    static void beginSingularCommands(VkCommandBuffer& commandBuffer,
                                      VkCommandPool& commandPool,
                                      VkQueue& queue);
    static void endSingularCommands(VkCommandBuffer& commandBuffer,
                                    VkCommandPool& commandPool,
                                    VkQueue& queue);
  };

  class Buffer {
   public:
    Buffer();
    virtual ~Buffer();

    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    void* mapped;

    static void create(VkDeviceSize size,
                       VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer& buffer,
                       VkDeviceMemory& bufferMemory);
    static void copy(VkBuffer srcBuffer,
                     VkBuffer dstBuffer,
                     VkDeviceSize size,
                     VkCommandBuffer& commandBuffer,
                     VkCommandPool& commandPool,
                     VkQueue& queue);
    static void copyToImage(VkBuffer buffer,
                            VkImage image,
                            uint32_t width,
                            uint32_t height,
                            VkCommandBuffer& commandBuffer,
                            VkCommandPool& commandPool,
                            VkQueue& queue);
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

    static void create(uint32_t width,
                       uint32_t height,
                       VkSampleCountFlagBits numSamples,
                       VkFormat format,
                       VkImageTiling tiling,
                       VkImageUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkImage& image,
                       VkDeviceMemory& imageMemory);
    static VkImageView createView(VkImage image,
                                  VkFormat format,
                                  VkImageAspectFlags aspectFlags);
    static void transitionLayout(VkCommandBuffer commandBuffer,
                                 VkImage image,
                                 VkFormat format,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout);
    static void loadTexture(const std::string& imagePath,
                            CE::Image& image,
                            VkCommandBuffer& commandBuffer,
                            VkCommandPool& commandPool,
                            VkQueue& queue);
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
    // void createDescriptorPool();
    // void allocateDescriptorSets();
    // void createDescriptorSets();
  };

  template <typename Checkresult, typename... Args>
  static void vulkanResult(Checkresult vkResult, Args&&... args);

  static uint32_t findMemoryType(uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties);
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
