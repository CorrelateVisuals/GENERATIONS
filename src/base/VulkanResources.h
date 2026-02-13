#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanCore.h"

namespace CE {

class Buffer {
public:
  VkBuffer buffer{};
  VkDeviceMemory memory{};
  void *mapped{};

  Buffer() = default;
  virtual ~Buffer();
  static void create(const VkDeviceSize &size,
                     const VkBufferUsageFlags &usage,
                     const VkMemoryPropertyFlags &properties,
                     Buffer &buffer);
  static void copy(const VkBuffer &srcBuffer,
                   VkBuffer &dstBuffer,
                   const VkDeviceSize size,
                   VkCommandBuffer &commandBuffer,
                   const VkCommandPool &commandPool,
                   const VkQueue &queue);
  static void copyToImage(const VkBuffer &buffer,
                          VkImage &image,
                          const uint32_t width,
                          const uint32_t height,
                          VkCommandBuffer &commandBuffer,
                          const VkCommandPool &commandPool,
                          const VkQueue &queue);
};

class Image {
public:
  VkImage image{};
  VkDeviceMemory memory{};
  VkImageView view{};
  VkSampler sampler{};
  std::string path{};
  VkImageCreateInfo info{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                         .pNext = nullptr,
                         .flags = 0,
                         .imageType = VK_IMAGE_TYPE_2D,
                         .format = VK_FORMAT_UNDEFINED,
                         .extent = {.width = 0, .height = 0, .depth = 1},
                         .mipLevels = 1,
                         .arrayLayers = 1,
                         .samples = VK_SAMPLE_COUNT_1_BIT,
                         .tiling = VK_IMAGE_TILING_OPTIMAL,
                         .usage = 0,
                         .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                         .queueFamilyIndexCount = 0,
                         .pQueueFamilyIndices = nullptr,
                         .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  Image() = default;
  Image(IMAGE_RESOURCE_TYPES image_type,
        const VkExtent2D &extent,
        const VkFormat format) {
    createResources(image_type, extent, format);
  }
  Image(const std::string &texturePath) {
    path = texturePath;
  }

  virtual ~Image() {
    destroyVulkanImages();
  };

  void create(const uint32_t width,
              const uint32_t height,
              const VkSampleCountFlagBits numSamples,
              const VkFormat format,
              const VkImageTiling tiling,
              const VkImageUsageFlags &usage,
              const VkMemoryPropertyFlags &properties);
  void recreate() {
    this->destroyVulkanImages();
  };
  void createView(const VkImageAspectFlags aspectFlags);
  void createSampler();
  void createResources(IMAGE_RESOURCE_TYPES imageType,
                       const VkExtent2D &dimensions,
                       const VkFormat format);
  void transitionLayout(const VkCommandBuffer &commandBuffer,
                        const VkFormat format,
                        const VkImageLayout oldLayout,
                        const VkImageLayout newLayout);
  void loadTexture(const std::string &imagePath,
                   const VkFormat format,
                   VkCommandBuffer &commandBuffer,
                   const VkCommandPool &commandPool,
                   const VkQueue &queue);
  static VkFormat findDepthFormat();

protected:
  static VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                      const VkImageTiling tiling,
                                      const VkFormatFeatureFlags &features);

private:
  void destroyVulkanImages() const;
};

} // namespace CE
