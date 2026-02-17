#pragma once

// Low-level GPU buffer/image primitives.
// Exists to encapsulate allocation, transfer, and image view/sampler lifecycle.

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace CE {

enum IMAGE_RESOURCE_TYPES { CE_DEPTH_IMAGE = 0, CE_MULTISAMPLE_IMAGE = 1 };

class BaseBuffer {
public:
  VkBuffer buffer{};
  VkDeviceMemory memory{};
  void *mapped{};

  BaseBuffer() = default;
  BaseBuffer(const BaseBuffer &) = delete;
  BaseBuffer &operator=(const BaseBuffer &) = delete;
  BaseBuffer(BaseBuffer &&) = delete;
  BaseBuffer &operator=(BaseBuffer &&) = delete;
  virtual ~BaseBuffer();
  static void create(const VkDeviceSize &size,
                     const VkBufferUsageFlags &usage,
                     const VkMemoryPropertyFlags &properties,
                     BaseBuffer &buffer);
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

class BaseImage {
public:
  VkImage image{};
  VkDeviceMemory memory{};
  VkImageView view{};
  VkSampler sampler{};
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

  BaseImage() = default;
  BaseImage(IMAGE_RESOURCE_TYPES image_type,
        const VkExtent2D &extent,
        const VkFormat format) {
    create_resources(image_type, extent, format);
  }
  BaseImage(const std::string &texture_path) {
    path = texture_path;
  }

  BaseImage(const BaseImage &) = delete;
  BaseImage &operator=(const BaseImage &) = delete;
  BaseImage(BaseImage &&) = delete;
  BaseImage &operator=(BaseImage &&) = delete;

  virtual ~BaseImage() {
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
  std::string path{};
  void destroy_vulkan_images();
};

} // namespace CE

constexpr CE::IMAGE_RESOURCE_TYPES CE_DEPTH_IMAGE = CE::CE_DEPTH_IMAGE;
constexpr CE::IMAGE_RESOURCE_TYPES CE_MULTISAMPLE_IMAGE = CE::CE_MULTISAMPLE_IMAGE;
