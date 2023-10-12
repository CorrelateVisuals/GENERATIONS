#pragma once
#include <vulkan/vulkan.h>

#include "Log.h"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace CE {

class Device {
 public:
  Device() = default;
  virtual ~Device() { destroyDevice(); }
  void destroyDevice();
  VkPhysicalDevice physical{VK_NULL_HANDLE};
  VkDevice logical{VK_NULL_HANDLE};
  const std::vector<const char*> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkPhysicalDeviceFeatures features{};
};

class LinkedDevice {
 public:
  static VkPhysicalDevice* _physical;
  static VkDevice* _logical;
  static void linkDevice(VkDevice* logicalDevice,
                         VkPhysicalDevice* physicalDevice);
};

class Queues {
 public:
  struct FamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const {
      return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
  };
  FamilyIndices familyIndices;
};

class Commands {
 public:
  VkCommandPool pool;
  static VkCommandBuffer singularCommandBuffer;

  static void beginSingularCommands(const VkCommandPool& commandPool,
                                    const VkQueue& queue);
  static void endSingularCommands(const VkCommandPool& commandPool,
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
                     Buffer& buffer);
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
  virtual ~Image() { destroyVulkanImages(); };
  void destroyVulkanImages();

  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
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

  void create(const uint32_t width,
              const uint32_t height,
              const VkSampleCountFlagBits numSamples,
              const VkFormat format,
              const VkImageTiling tiling,
              const VkImageUsageFlags& usage,
              const VkMemoryPropertyFlags properties);
  void recreate() { this->destroyVulkanImages(); };
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
  void createColorResources(const VkExtent2D& dimensions,
                            const VkFormat format,
                            const VkSampleCountFlagBits samples);
  void createDepthResources(const VkExtent2D& dimensions,
                            const VkFormat format,
                            const VkSampleCountFlagBits samples);
};

class Swapchain {
 public:
  Swapchain() = default;
  ~Swapchain() { destroySwapchain(); };
  void destroySwapchain();
  std::vector<CE::Image> images;
  std::vector<VkFramebuffer> framebuffers;
  VkSwapchainKHR swapChain;
  VkFormat imageFormat;
  VkExtent2D extent;
  struct SupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };
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
};  // namespace CE

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
