#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanDescriptor.h"
#include "VulkanSync.h"

namespace CE {

struct PushConstants {
  VkShaderStageFlags shaderStage{};
  uint32_t count{};
  uint32_t offset{};
  uint32_t size{};
  std::array<uint64_t, 32> data{};

  PushConstants(VkShaderStageFlags stage, uint32_t dataSize, uint32_t dataOffset);
  virtual ~PushConstants() = default;
  void setData(const uint64_t &data);
};

class PipelineLayout {
public:
  VkPipelineLayout layout{};

  PipelineLayout() = default;
  virtual ~PipelineLayout();
  void createLayout(const VkDescriptorSetLayout &setLayout);
  void createLayout(const VkDescriptorSetLayout &setLayout,
                    const PushConstants &_pushConstants);
};

class RenderPass {
public:
  VkRenderPass renderPass{};

  RenderPass() = default;
  virtual ~RenderPass();
  void create(VkSampleCountFlagBits msaaImageSamples, VkFormat swapchainImageFormat);
  void createFramebuffers(CE::Swapchain &swapchain,
                          const VkImageView &msaaView,
                          const VkImageView &depthView) const;
};

class PipelinesConfiguration {
public:
#define PIPELINE_OBJECTS                                                                 \
  VkPipeline pipeline{};                                                                 \
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
  std::unordered_map<std::string, std::variant<Graphics, Compute>> pipelineMap{};

  PipelinesConfiguration() = default;
  virtual ~PipelinesConfiguration();
  void createPipelines(VkRenderPass &renderPass,
                       const VkPipelineLayout &graphicsLayout,
                       const VkPipelineLayout &computeLayout,
                       VkSampleCountFlagBits &msaaSamples);
  const std::vector<std::string> &getPipelineShadersByName(const std::string &name);
  VkPipeline &getPipelineObjectByName(const std::string &name);
  const std::array<uint32_t, 3> &getWorkGroupsByName(const std::string &name);

protected:
  void compileShaders();

private:
  bool setShaderStages(const std::string &pipelineName,
                       std::vector<VkPipelineShaderStageCreateInfo> &shaderStages);
  std::vector<char> readShaderFile(const std::string &filename);
  VkPipelineShaderStageCreateInfo createShaderModules(VkShaderStageFlagBits shaderStage,
                                                      std::string shaderName);
  void destroyShaderModules();
};

} // namespace CE
