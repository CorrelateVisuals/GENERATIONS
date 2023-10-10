#pragma once
#include <vulkan/vulkan.h>

#include <iostream>
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
                                      const VkCommandPool& commandPool,
                                      const VkQueue& queue);
    static void endSingularCommands(const VkCommandBuffer& commandBuffer,
                                    const VkCommandPool& commandPool,
                                    const VkQueue& queue);
  };

  class Buffer {
   public:
    Buffer();
    virtual ~Buffer();

    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped;

    static void create(const VkDeviceSize size,
                       const VkBufferUsageFlags usage,
                       const VkMemoryPropertyFlags properties,
                       VkBuffer& buffer,
                       VkDeviceMemory& memory);
    static void copy(const VkBuffer& srcBuffer,
                     VkBuffer& dstBuffer,
                     const VkDeviceSize size,
                     VkCommandBuffer& commandBuffer,
                     const VkCommandPool& commandPool,
                     const VkQueue& queue);
    static void copyToImage(const VkBuffer buffer,
                            VkImage& image,
                            const uint32_t width,
                            const uint32_t height,
                            VkCommandBuffer& commandBuffer,
                            const VkCommandPool& commandPool,
                            const VkQueue& queue);
  };

  class Image {
   public:
    Image();
    virtual ~Image();

    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkSampleCountFlagBits sampleCount;
    VkFormat format;

    void create(const uint32_t width,
                const uint32_t height,
                const VkSampleCountFlagBits numSamples,
                const VkFormat format,
                const VkImageTiling tiling,
                const VkImageUsageFlags& usage,
                const VkMemoryPropertyFlags properties);
    void recreate() { this->~Image(); };
    void createView(const VkImageAspectFlags aspectFlags);
    void createSampler();
    void transitionLayout(const VkCommandBuffer& commandBuffer,
                          const VkFormat format,
                          const VkImageLayout oldLayout,
                          const VkImageLayout newLayout);
    void loadTexture(const std::string& imagePath,
                     const VkFormat format,
                     VkCommandBuffer& commandBuffer,
                     const VkCommandPool& commandPool,
                     const VkQueue& queue);
    static VkFormat findDepthFormat();
    static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling,
                                        VkFormatFeatureFlags features);
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
