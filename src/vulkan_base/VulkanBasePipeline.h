#pragma once

// Pipeline/render-pass abstractions used by graphics + compute setup.
// Exists to separate pipeline compilation/state assembly from frame execution.

#include <array>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanBaseDescriptor.h"
#include "VulkanBaseSync.h"

namespace CE {

struct BasePushConstants {
  VkShaderStageFlags shader_stage{};
  uint32_t count{};
  uint32_t offset{};
  uint32_t size{};
  std::array<uint64_t, 32> data{};

  BasePushConstants(VkShaderStageFlags stage, uint32_t data_size, uint32_t data_offset);
  virtual ~BasePushConstants() = default;
  void set_data(const uint64_t &value);
  void set_data(uint32_t value, float fraction);
  void set_data(const uint64_t &value, float fraction);
};

class BasePipelineLayout {
public:
  VkPipelineLayout layout{};

  BasePipelineLayout() = default;
  BasePipelineLayout(const BasePipelineLayout &) = delete;
  BasePipelineLayout &operator=(const BasePipelineLayout &) = delete;
  BasePipelineLayout(BasePipelineLayout &&) = delete;
  BasePipelineLayout &operator=(BasePipelineLayout &&) = delete;
  virtual ~BasePipelineLayout();
  void create_layout(const VkDescriptorSetLayout &set_layout);
  void create_layout(const VkDescriptorSetLayout &set_layout,
                     const BasePushConstants &push_constants);
};

class BaseRenderPass {
public:
  VkRenderPass render_pass{};

  BaseRenderPass() = default;
  BaseRenderPass(const BaseRenderPass &) = delete;
  BaseRenderPass &operator=(const BaseRenderPass &) = delete;
  BaseRenderPass(BaseRenderPass &&) = delete;
  BaseRenderPass &operator=(BaseRenderPass &&) = delete;
  virtual ~BaseRenderPass();
  void create(VkSampleCountFlagBits msaa_image_samples, VkFormat swapchain_image_format);
  void create_framebuffers(CE::BaseSwapchain &swapchain,
                           const VkImageView &msaa_view,
                           const VkImageView &depth_view) const;
};

class BasePipelinesConfiguration {
public:
  struct Graphics {
    VkPipeline pipeline{};
    std::vector<std::string> shaders{};
    std::vector<VkVertexInputAttributeDescription> vertex_attributes{};
    std::vector<VkVertexInputBindingDescription> vertex_bindings{};
  };
  struct Compute {
    VkPipeline pipeline{};
    std::vector<std::string> shaders{};
    std::array<uint32_t, 3> work_groups{};
  };

  BasePipelinesConfiguration() = default;
  BasePipelinesConfiguration(const BasePipelinesConfiguration &) = delete;
  BasePipelinesConfiguration &operator=(const BasePipelinesConfiguration &) = delete;
  BasePipelinesConfiguration(BasePipelinesConfiguration &&) = delete;
  BasePipelinesConfiguration &operator=(BasePipelinesConfiguration &&) = delete;
  virtual ~BasePipelinesConfiguration();
  void create_pipelines(VkRenderPass &render_pass,
                        const VkPipelineLayout &graphics_layout,
                        const VkPipelineLayout &compute_layout,
                        VkSampleCountFlagBits &msaa_samples);
  VkPipeline &get_pipeline_object_by_name(const std::string &name);
  const std::array<uint32_t, 3> &get_work_groups_by_name(const std::string &name);

protected:
  std::unordered_map<std::string, std::variant<Graphics, Compute>> pipeline_map{};
  void compile_shaders();

private:
  std::vector<VkShaderModule> shader_modules{};
  const std::string shader_dir = "shaders/";
  const std::vector<std::string> &get_pipeline_shaders_by_name(const std::string &name);
  bool set_shader_stages(const std::string &pipeline_name,
                         std::vector<VkPipelineShaderStageCreateInfo> &shader_stages);
  std::vector<char> read_shader_file(const std::string &filename);
  VkPipelineShaderStageCreateInfo create_shader_modules(VkShaderStageFlagBits shader_stage,
                                                        std::string shader_name);
  void destroy_shader_modules();
};

} // namespace CE
