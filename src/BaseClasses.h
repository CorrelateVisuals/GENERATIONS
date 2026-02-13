#pragma once
#include <vulkan/vulkan.h>

#include "Log.h"
#include "ValidationLayers.h"
#include "Window.h"
#include "base/VulkanCore.h"
#include "base/VulkanPipelinePresets.h"
#include "base/VulkanUtils.h"

#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

class VulkanMechanics;
class Resources;
class Pipelines;

namespace CE {
class Swapchain;

// Mechanics
class Queues {
 public:
  VkQueue graphics{};
  VkQueue compute{};
  VkQueue present{};
  struct FamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily{};
    std::optional<uint32_t> presentFamily{};
    bool isComplete() const {
      return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
  };
  FamilyIndices familyIndices{};

  Queues() = default;
  virtual ~Queues() = default;
  FamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice,
                                  const VkSurfaceKHR& surface) const;
};

class InitializeVulkan {
 public:
  VkSurfaceKHR surface{};
  VkInstance instance{};
  ValidationLayers validation{};

  InitializeVulkan();
  virtual ~InitializeVulkan();

 private:
  void createInstance();
  void createSurface(GLFWwindow* window);
  std::vector<const char*> getRequiredExtensions() const;
};

class Device {
 public:
  VkPhysicalDevice physical{VK_NULL_HANDLE};
  VkPhysicalDeviceFeatures features{};
  VkSampleCountFlagBits maxUsableSampleCount{VK_SAMPLE_COUNT_1_BIT};
  VkDevice logical{VK_NULL_HANDLE};

  static Device* baseDevice;

  Device() = default;
  virtual ~Device() { destroyDevice(); }

 protected:
  void pickPhysicalDevice(const InitializeVulkan& initVulkan,
                          Queues& queues,
                          Swapchain& swapchain);
  void createLogicalDevice(const InitializeVulkan& initVulkan, Queues& queues);
  void destroyDevice();

 private:
  VkPhysicalDeviceProperties properties{};
  std::vector<const char*> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  static std::vector<VkDevice> destroyedDevices;

    std::vector<VkDeviceQueueCreateInfo> fillQueueCreateInfos(
      const Queues& queues) const;
    VkDeviceCreateInfo getDeviceCreateInfo(
      const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos) const;
  void setValidationLayers(const InitializeVulkan& initVulkan,
                           VkDeviceCreateInfo& createInfo);
    std::vector<VkPhysicalDevice> fillDevices(
      const InitializeVulkan& initVulkan) const;
    bool isDeviceSuitable(const VkPhysicalDevice& physical,
              Queues& queues,
              const InitializeVulkan& initVulkan,
              Swapchain& swapchain);
  void getMaxUsableSampleCount();
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& physical) const;
};

class CommandBuffers {
 public:
  VkCommandPool pool{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> graphics{};
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> compute{};
  static VkCommandBuffer singularCommandBuffer;

  CommandBuffers() = default;
  virtual ~CommandBuffers();
  static void beginSingularCommands(const VkCommandPool& commandPool,
                                    const VkQueue& queue);
  static void endSingularCommands(const VkCommandPool& commandPool,
                                  const VkQueue& queue);
  virtual void recordComputeCommandBuffer(Resources& resources,
                                          Pipelines& pipelines,
                                          const uint32_t imageIndex) = 0;
  virtual void recordGraphicsCommandBuffer(Swapchain& swapchain,
                                           Resources& resources,
                                           Pipelines& pipelines,
                                           const uint32_t imageIndex) = 0;

 protected:
  void createPool(const Queues::FamilyIndices& familyIndices);
  void createBuffers(
      std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>& commandBuffers) const;
};

struct CommandInterface {
  VkCommandBuffer& commandBuffer;
  const VkCommandPool& commandPool;
  const VkQueue& queue;

  CommandInterface(VkCommandBuffer& commandBufferRef,
                   const VkCommandPool& commandPoolRef,
                   const VkQueue& queueRef)
      : commandBuffer(commandBufferRef),
        commandPool(commandPoolRef),
        queue(queueRef) {}
};

class Buffer {
 public:
  VkBuffer buffer{};
  VkDeviceMemory memory{};
  void* mapped{};

  Buffer() = default;
  virtual ~Buffer();
  static void create(const VkDeviceSize& size,
                     const VkBufferUsageFlags& usage,
                     const VkMemoryPropertyFlags& properties,
                     Buffer& buffer);
  static void copy(const VkBuffer& srcBuffer,
                   VkBuffer& dstBuffer,
                   const VkDeviceSize size,
                   VkCommandBuffer& commandBuffer,
                   const VkCommandPool& commandPool,
                   const VkQueue& queue);
  static void copyToImage(const VkBuffer& buffer,
                          VkImage& image,
                          const uint32_t width,
                          const uint32_t height,
                          VkCommandBuffer& commandBuffer,
                          const VkCommandPool& commandPool,
                          const VkQueue& queue);
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
        const VkExtent2D& extent,
        const VkFormat format) {
    createResources(image_type, extent, format);
  }
  Image(const std::string& texturePath) { path = texturePath; }

  virtual ~Image() { destroyVulkanImages(); };

  void create(const uint32_t width,
              const uint32_t height,
              const VkSampleCountFlagBits numSamples,
              const VkFormat format,
              const VkImageTiling tiling,
              const VkImageUsageFlags& usage,
              const VkMemoryPropertyFlags& properties);
  void recreate() { this->destroyVulkanImages(); };
  void createView(const VkImageAspectFlags aspectFlags);
  void createSampler();
  void createResources(IMAGE_RESOURCE_TYPES imageType,
                       const VkExtent2D& dimensions,
                       const VkFormat format);
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

 protected:
  static VkFormat findSupportedFormat(
      const std::vector<VkFormat>& candidates,
      const VkImageTiling tiling,
      const VkFormatFeatureFlags& features);

 private:
  void destroyVulkanImages() const;
};

class SynchronizationObjects {
 public:
  SynchronizationObjects() = default;
  ~SynchronizationObjects() { destroy(); };

  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{};
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{};
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> computeFinishedSemaphores{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> graphicsInFlightFences{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> computeInFlightFences{};
  uint32_t currentFrame = 0;

 protected:
  void create();

 private:
  void destroy() const;
};

class Swapchain {
 public:
  VkSwapchainKHR swapchain{};
  VkExtent2D extent{};
  VkFormat imageFormat{};
  std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> images{};
  std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> framebuffers{};

  struct SupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
  } supportDetails{};

  Swapchain() = default;
  virtual ~Swapchain() { destroy(); };
  SupportDetails checkSupport(const VkPhysicalDevice& physicalDevice,
                              const VkSurfaceKHR& surface);

 protected:
  void create(const VkSurfaceKHR& surface, const Queues& queues);
  void recreate(const VkSurfaceKHR& surface,
                const Queues& queues,
                SynchronizationObjects& syncObjects);

 private:
  void destroy();
    VkSurfaceFormatKHR pickSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR pickPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D pickExtent(
      GLFWwindow* window,
      const VkSurfaceCapabilitiesKHR& capabilities) const;
    uint32_t getImageCount(const Swapchain::SupportDetails& swapchainSupport)
      const;
};

// Resources
class DescriptorInterface {
public:
    size_t writeIndex{};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets{};
    VkDescriptorSetLayout setLayout{};
    std::array<VkDescriptorSetLayoutBinding, NUM_DESCRIPTORS> setLayoutBindings{};
    std::array<std::array<VkWriteDescriptorSet, NUM_DESCRIPTORS>,
        MAX_FRAMES_IN_FLIGHT>
        descriptorWrites{};
    std::vector<VkDescriptorPoolSize> poolSizes{};

    DescriptorInterface() = default;
    virtual ~DescriptorInterface();

    void initialzeSets();
    void updateSets();

private:
    VkDescriptorPool pool;

    void createSetLayout();
    void createPool();
    void allocateSets();
};

class Descriptor {
public:
    Descriptor() = default;
    virtual ~Descriptor() {};

protected:
    size_t myIndex{ 0 };
    VkDescriptorPoolSize poolSize{};
    VkDescriptorSetLayoutBinding setLayoutBinding{};
    struct DescriptorInformation {
        std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> previousFrame{};
        std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> currentFrame{};
    } info;
};

struct PushConstants {
  VkShaderStageFlags shaderStage{};
  uint32_t count{};
  uint32_t offset{};
  uint32_t size{};
  std::array<uint64_t, 32> data{};

  PushConstants(VkShaderStageFlags stage,
                uint32_t dataSize,
                uint32_t dataOffset);
  virtual ~PushConstants() = default;
  void setData(const uint64_t& data);
};

// Pipelines
class PipelineLayout {
 public:
  VkPipelineLayout layout{};

  PipelineLayout() = default;
  virtual ~PipelineLayout();
  void createLayout(const VkDescriptorSetLayout& setLayout);
  void createLayout(const VkDescriptorSetLayout& setLayout,
                    const PushConstants& _pushConstants);
};

class RenderPass {
 public:
  VkRenderPass renderPass{};

  RenderPass() = default;
  virtual ~RenderPass();
  void create(VkSampleCountFlagBits msaaImageSamples,
              VkFormat swapchainImageFormat);
  void createFramebuffers(CE::Swapchain& swapchain,
                          const VkImageView& msaaView,
                          const VkImageView& depthView) const;
};

class PipelinesConfiguration {
 public:
#define PIPELINE_OBJECTS \
  VkPipeline pipeline{}; \
  std::vector<std::string> shaders{};

  struct Graphics {
    PIPELINE_OBJECTS
    std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
    std::vector<VkVertexInputBindingDescription> vertexBindings{};
  };
  struct Compute {
    PIPELINE_OBJECTS
    std::array<uint32_t, 3> workGroups{};
  };
#undef PIPELINE_OBJECTS

  std::vector<VkShaderModule> shaderModules{};
  const std::string shaderDir = "shaders/";
  std::unordered_map<std::string, std::variant<Graphics, Compute>>
      pipelineMap{};

  PipelinesConfiguration() = default;
  virtual ~PipelinesConfiguration();
  void createPipelines(VkRenderPass& renderPass,
                       const VkPipelineLayout& graphicsLayout,
                       const VkPipelineLayout& computeLayout,
                       VkSampleCountFlagBits& msaaSamples);
  const std::vector<std::string>& getPipelineShadersByName(
      const std::string& name);
  VkPipeline& getPipelineObjectByName(const std::string& name);
  const std::array<uint32_t, 3>& getWorkGroupsByName(const std::string& name);

 protected:
  void compileShaders();

 private:
  bool setShaderStages(
      const std::string& pipelineName,
      std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);
  std::vector<char> readShaderFile(const std::string& filename);
  VkPipelineShaderStageCreateInfo createShaderModules(
      VkShaderStageFlagBits shaderStage,
      std::string shaderName);
  void destroyShaderModules();
};

};  // namespace CE
