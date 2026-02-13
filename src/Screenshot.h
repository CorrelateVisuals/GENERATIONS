#pragma once
#include <vulkan/vulkan.h>

#include "BaseClasses.h"

#include <string>

namespace CE {

class Screenshot {
 public:
  Screenshot() = delete;
  ~Screenshot() = delete;

  static void capture(const VkImage& srcImage,
                      const VkExtent2D& extent,
                      const VkFormat& format,
                      const VkCommandPool& commandPool,
                      const VkQueue& queue,
                      const std::string& filename);

 private:
  static void copyImageToBuffer(const VkImage& srcImage,
                                Buffer& dstBuffer,
                                const VkExtent2D& extent,
                                const VkCommandPool& commandPool,
                                const VkQueue& queue);

  static void saveBufferToFile(const Buffer& buffer,
                               const VkExtent2D& extent,
                               const VkFormat& format,
                               const std::string& filename);
};

}  // namespace CE
