#pragma once
#include <vulkan/vulkan.h>

#include "base/VulkanDevice.h"
#include "base/VulkanResources.h"
#include "base/VulkanSync.h"

#include <string>

namespace CE {

class Screenshot {
public:
  Screenshot() = delete;
  ~Screenshot() = delete;

  static void capture(const VkImage &srcImage,
                      const VkExtent2D &extent,
                      const VkFormat &format,
                      const VkCommandPool &command_pool,
                      const VkQueue &queue,
                      const std::string &filename);

private:
  static void copy_image_to_buffer(const VkImage &src_image,
                                   Buffer &dst_buffer,
                                const VkExtent2D &extent,
                                const VkCommandPool &command_pool,
                                const VkQueue &queue);

  static void save_buffer_to_file(const Buffer &buffer,
                                  const VkExtent2D &extent,
                                  const VkFormat &format,
                                  const std::string &filename);
};

} // namespace CE
