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
  VkShaderStageFlags shader_stage{};
  uint32_t count{};
  uint32_t offset{};
  uint32_t size{};
  std::array<uint64_t, 32> data{};

  PushConstants(VkShaderStageFlags stage, uint32_t data_size, uint32_t data_offset);
  virtual ~PushConstants() = default;
  void set_data(const uint64_t &value);
};

class PipelineLayout {
public:
  VkPipelineLayout layout{};

  PipelineLayout() = default;
  virtual ~PipelineLayout();
  void create_layout(const VkDescriptorSetLayout &set_layout);
  void create_layout(const VkDescriptorSetLayout &set_layout,
                     const PushConstants &push_constants);
};

class RenderPass {
public:
  VkRenderPass render_pass{};

  RenderPass() = default;
  virtual ~RenderPass();
  void create(VkSampleCountFlagBits msaa_image_samples, VkFormat swapchain_image_format);
  void create_framebuffers(CE::Swapchain &swapchain,
                           const VkImageView &msaa_view,
                           const VkImageView &depth_view) const;
};

class PipelinesConfiguration {
public:
#define PIPELINE_OBJECTS                                                                 \
  VkPipeline pipeline{};                                                                 \
  std::vector<std::string> shaders{};

  struct Graphics {
    PIPELINE_OBJECTS
    std::vector<VkVertexInputAttributeDescription> vertex_attributes{};
    std::vector<VkVertexInputBindingDescription> vertex_bindings{};
  };
  struct Compute {
    PIPELINE_OBJECTS
    std::array<uint32_t, 3> work_groups{};
  };
#undef PIPELINE_OBJECTS

  PipelinesConfiguration() = default;
  virtual ~PipelinesConfiguration();
  void create_pipelines(VkRenderPass &render_pass,
                        const VkPipelineLayout &graphics_layout,
                        const VkPipelineLayout &compute_layout,
                        VkSampleCountFlagBits &msaa_samples);
  const std::vector<std::string> &get_pipeline_shaders_by_name(const std::string &name);
  VkPipeline &get_pipeline_object_by_name(const std::string &name);
  const std::array<uint32_t, 3> &get_work_groups_by_name(const std::string &name);

protected:
  std::unordered_map<std::string, std::variant<Graphics, Compute>> pipeline_map{};
  void compile_shaders();

private:
  std::vector<VkShaderModule> shader_modules{};
  const std::string shader_dir = "shaders/";
  bool set_shader_stages(const std::string &pipeline_name,
                         std::vector<VkPipelineShaderStageCreateInfo> &shader_stages);
  std::vector<char> read_shader_file(const std::string &filename);
  VkPipelineShaderStageCreateInfo create_shader_modules(VkShaderStageFlagBits shader_stage,
                                                        std::string shader_name);
  void destroy_shader_modules();
};

} // namespace CE
