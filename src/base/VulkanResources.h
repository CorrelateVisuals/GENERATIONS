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
  static void copy(const VkBuffer &src_buffer,
                   VkBuffer &dst_buffer,
                   const VkDeviceSize size,
                   VkCommandBuffer &command_buffer,
                   const VkCommandPool &command_pool,
                   const VkQueue &queue);
  static void copy_to_image(const VkBuffer &buffer,
                            VkImage &image,
                            const uint32_t width,
                            const uint32_t height,
                            VkCommandBuffer &command_buffer,
                            const VkCommandPool &command_pool,
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
    create_resources(image_type, extent, format);
  }
  Image(const std::string &texture_path) {
    path = texture_path;
  }

  virtual ~Image() {
    destroy_vulkan_images();
  };

  void create(const uint32_t width,
              const uint32_t height,
              const VkSampleCountFlagBits num_samples,
              const VkFormat format,
              const VkImageTiling tiling,
              const VkImageUsageFlags &usage,
              const VkMemoryPropertyFlags &properties);
  void recreate() {
    this->destroy_vulkan_images();
  };
  void create_view(const VkImageAspectFlags aspect_flags);
  void create_sampler();
  void create_resources(IMAGE_RESOURCE_TYPES image_type,
                        const VkExtent2D &dimensions,
                        const VkFormat format);
  void transition_layout(const VkCommandBuffer &command_buffer,
                         const VkFormat format,
                         const VkImageLayout old_layout,
                         const VkImageLayout new_layout);
  void load_texture(const std::string &image_path,
                    const VkFormat format,
                    VkCommandBuffer &command_buffer,
                    const VkCommandPool &command_pool,
                    const VkQueue &queue);
  static VkFormat find_depth_format();

protected:
  static VkFormat find_supported_format(const std::vector<VkFormat> &candidates,
                                        const VkImageTiling tiling,
                                        const VkFormatFeatureFlags &features);

private:
  void destroy_vulkan_images() const;
};

} // namespace CE
