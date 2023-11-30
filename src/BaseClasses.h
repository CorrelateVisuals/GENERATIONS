#pragma once
#include <vulkan/vulkan.h>

#include "Log.h"
#include "ValidationLayers.h"
#include "Window.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr size_t NUM_DESCRIPTORS = 5;

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
  const FamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice,
                                  const VkSurfaceKHR& surface)const;
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

  const std::vector<VkDeviceQueueCreateInfo> fillQueueCreateInfos(
      const Queues& queues) const;
  const VkDeviceCreateInfo getDeviceCreateInfo(
      const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos) const;
  void setValidationLayers(const InitializeVulkan& initVulkan,
                           VkDeviceCreateInfo& createInfo);
  const std::vector<VkPhysicalDevice> fillDevices(
      const InitializeVulkan& initVulkan) const;
  const bool isDeviceSuitable(const VkPhysicalDevice& physical,
                              Queues& queues,
                              const InitializeVulkan& initVulkan,
                              Swapchain& swapchain);
  void getMaxUsableSampleCount();
  const bool checkDeviceExtensionSupport(
      const VkPhysicalDevice& physical) const;

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
  void createResources(const VkExtent2D& dimensions,
                       const VkFormat format,
                       const VkImageUsageFlags usage,
                       const VkImageAspectFlagBits aspect);
  void transitionLayout(const VkCommandBuffer& commandBuffer,
                        const VkFormat format,
                        const VkImageLayout oldLayout,
                        const VkImageLayout newLayout);
  void loadTexture(const std::string& imagePath,
                   const VkFormat format,
                   VkCommandBuffer& commandBuffer,
                   const VkCommandPool& commandPool,
                   const VkQueue& queue);
  static const VkFormat findDepthFormat();

 protected:
  static const VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
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
  const SupportDetails checkSupport(const VkPhysicalDevice& physicalDevice,
                              const VkSurfaceKHR& surface);

 protected:
  void create(const VkSurfaceKHR& surface, const Queues& queues);
  void recreate(const VkSurfaceKHR& surface,
                const Queues& queues,
                SynchronizationObjects& syncObjects);

 private:
  void destroy();
  const VkSurfaceFormatKHR pickSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  const VkPresentModeKHR pickPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  const VkExtent2D pickExtent(GLFWwindow* window,
                        const VkSurfaceCapabilitiesKHR& capabilities) const;
  const uint32_t getImageCount( const Swapchain::SupportDetails& swapchainSupport) const;
};

// Resources
class Descriptor {
 public:
  static std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets;
  static VkDescriptorSetLayout setLayout;
  static std::array<VkDescriptorSetLayoutBinding, NUM_DESCRIPTORS>
      setLayoutBindings;
  static std::array<std::array<VkWriteDescriptorSet, NUM_DESCRIPTORS>,
                    MAX_FRAMES_IN_FLIGHT>
      descriptorWrites;

  Descriptor() = default;
  virtual ~Descriptor();
  static void createSetLayout(
      const std::array<VkDescriptorSetLayoutBinding, NUM_DESCRIPTORS>&
          layoutBindings);
  static void createPool();
  static void allocateSets();
  static void updateSets(
      const std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>& sets,
      std::array<std::array<VkWriteDescriptorSet, NUM_DESCRIPTORS>,
                 MAX_FRAMES_IN_FLIGHT>& descriptorWrites);

 protected:
  size_t myIndex{0};
  static size_t writeIndex;
  static VkDescriptorPool pool;
  VkDescriptorPoolSize poolSize{};
  static std::vector<VkDescriptorPoolSize> poolSizes;
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

  PushConstants() = default;
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
                          const VkImageView& depthView);
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
  std::vector<std::string>& getPipelineShadersByName(const std::string& name);
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

template <typename Checkresult, typename... Args>
static void VULKAN_RESULT(Checkresult vkResult, Args&&... args);

static const uint32_t findMemoryType(const uint32_t typeFilter,
                                     const VkMemoryPropertyFlags properties);

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
void CE::VULKAN_RESULT(Checkresult vkResult, Args&&... args) {
  using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
  std::string objectName = typeid(ObjectType).name();

  VkResult result = vkResult(std::forward<Args>(args)...);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName +
                             "!");
  }
}
