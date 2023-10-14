#pragma once
#include <vulkan/vulkan.h>

#include "Log.h"
#include "Window.h"

#include <iostream>
#include <memory>
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

class UsableDevices {
 public:
  VkPhysicalDevice physical;
  VkDevice logical;
  void attach(CE::Device& device);
};
extern std::shared_ptr<UsableDevices> baseDevice;

class Queues {
 public:
  Queues() = default;
  virtual ~Queues() = default;

  struct FamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const {
      return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
  };
  FamilyIndices familyIndices;
  FamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice,
                                  const VkSurfaceKHR& surface);
};

class Commands {
 public:
  Commands() = default;
  virtual ~Commands();
  VkCommandPool pool;
  static VkCommandBuffer singularCommandBuffer;

  void createCommandPool(const Queues::FamilyIndices& familyIndices);
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
              const VkMemoryPropertyFlags& properties);
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
                                      const VkImageTiling tiling,
                                      const VkFormatFeatureFlags& features);
  void createColorResources(const VkExtent2D& dimensions,
                            const VkFormat format,
                            const VkSampleCountFlagBits samples);
  void createDepthResources(const VkExtent2D& dimensions,
                            const VkFormat format,
                            const VkSampleCountFlagBits samples);
};

struct SynchronizationObjects {
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkSemaphore> computeFinishedSemaphores;
  std::vector<VkFence> graphicsInFlightFences;
  std::vector<VkFence> computeInFlightFences;
  uint32_t currentFrame = 0;
};

class Swapchain {
 public:
  Swapchain() = default;
  virtual ~Swapchain() = default;

  VkSwapchainKHR swapchain;
  VkExtent2D extent;
  VkFormat imageFormat;
  std::vector<CE::Image> images;
  std::vector<VkFramebuffer> framebuffers;

  struct SupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  } supportDetails;

  void create(const VkSurfaceKHR& surface, const Queues& queues);
  void recreate(const VkSurfaceKHR& surface,
                const Queues& queues,
                SynchronizationObjects& syncObjects);
  void destroy();

  SupportDetails checkSupport(const VkPhysicalDevice& physicalDevice,
                              const VkSurfaceKHR& surface);
  VkSurfaceFormatKHR pickSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR pickPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);
  VkExtent2D pickExtent(GLFWwindow* window,
                        const VkSurfaceCapabilitiesKHR& capabilities);
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

// Pipeline Presets
constexpr static inline VkPipelineRasterizationStateCreateInfo
    rasterizationCullBackBit{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_TRUE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_TRUE,
        .depthBiasConstantFactor = 0.1f,
        .depthBiasClamp = 0.01f,
        .depthBiasSlopeFactor = 0.02f,
        .lineWidth = 1.0f};

constexpr static inline VkPipelineInputAssemblyStateCreateInfo
    inputAssemblyStateTriangleList{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

constexpr static inline VkPipelineVertexInputStateCreateInfo
    vertexInputStateDefault{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr};

constexpr static inline VkPipelineMultisampleStateCreateInfo
    multisampleStateDefault{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_TRUE,
        .minSampleShading = 1.0f};

constexpr static inline VkPipelineDepthStencilStateCreateInfo
    depthStencilStateDefault{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateFalse{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateMultiply{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateAdd{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateAverage{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateSubtract{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendAttachmentState
    colorBlendAttachmentStateScreen{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

constexpr static inline VkPipelineColorBlendStateCreateInfo
    colorBlendStateDefault{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = nullptr,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

constexpr static inline VkPipelineViewportStateCreateInfo viewportStateDefault{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1};

constexpr static inline VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
constexpr static inline VkPipelineDynamicStateCreateInfo dynamicStateDefault{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
    .pDynamicStates = dynamicStates};

constexpr static inline VkPipelineLayoutCreateInfo layoutDefault{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = nullptr};

const static inline uint32_t tessellationTopologyTriangle = 3;
constexpr static inline VkPipelineTessellationStateCreateInfo
    tessellationStateDefault{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = tessellationTopologyTriangle};
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
