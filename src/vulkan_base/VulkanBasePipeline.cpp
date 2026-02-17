#include "VulkanBasePipeline.h"
#include "VulkanBasePipelinePresets.h"
#include "VulkanBaseUtils.h"

#include "library/Library.h"
#include "engine/Log.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

std::string resolve_shader_spv_path(const std::string &shader_dir,
                                    const std::string &shader_name) {
  const std::string alias_path = shader_dir + shader_name;

  if (!shader_name.ends_with(".spv")) {
    return alias_path;
  }

  const std::string base = shader_name.substr(0, shader_name.size() - 4);
  const std::array<std::pair<std::string_view, std::string_view>, 6> stage_suffix = {{
      {"Comp", "comp"},
      {"Vert", "vert"},
      {"Tesc", "tesc"},
      {"Tese", "tese"},
      {"Frag", "frag"},
      {"Geom", "geom"},
  }};

  for (const auto &[token, extension] : stage_suffix) {
    if (base.ends_with(token)) {
      const std::string source_base = base.substr(0, base.size() - token.size());
      const std::string canonical_path = shader_dir + source_base + "." +
                                         std::string(extension) + ".spv";
      if (std::filesystem::exists(canonical_path)) {
        return canonical_path;
      }
      break;
    }
  }

  return alias_path;
}

} // namespace

CE::BaseRenderPass::~BaseRenderPass() {
  Log::text("{ []< }", "destructing Render Pass");
  if (BaseDevice::base_device) {
    vkDestroyRenderPass(BaseDevice::base_device->logical_device, this->render_pass, nullptr);
  }
}

void CE::BaseRenderPass::create(VkSampleCountFlagBits msaa_image_samples,
                            VkFormat swapchain_image_format) {
  Log::text("{ []< }", "Render Pass");
  Log::text(Log::Style::char_leader,
            "colorAttachment, depthAttachment, colorAttachmentResolve");

  VkAttachmentDescription colorAttachment{
      .format = swapchain_image_format,
      .samples = msaa_image_samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription depthAttachment{
      .format = CE::BaseImage::find_depth_format(),
      .samples = msaa_image_samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription colorAttachmentResolve{
      .format = swapchain_image_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

  VkAttachmentReference colorAttachmentRef{
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentReference depthAttachmentRef{
      .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentReference colorAttachmentResolveRef{
      .attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                               .colorAttachmentCount = 1,
                               .pColorAttachments = &colorAttachmentRef,
                               .pResolveAttachments = &colorAttachmentResolveRef,
                               .pDepthStencilAttachment = &depthAttachmentRef};

  VkSubpassDependency dependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

  std::vector<VkAttachmentDescription> attachments = {
      colorAttachment, depthAttachment, colorAttachmentResolve};

  VkRenderPassCreateInfo render_pass_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  CE::vulkan_result(vkCreateRenderPass,
                    BaseDevice::base_device->logical_device,
                    &render_pass_info,
                    nullptr,
                    &this->render_pass);
}

void CE::BaseRenderPass::create_framebuffers(CE::BaseSwapchain &swapchain,
                                         const VkImageView &msaa_view,
                                         const VkImageView &depth_view) const {
  Log::text("{ 101 }", "Frame Buffers:", swapchain.images.size());

  Log::text(Log::Style::char_leader,
            "attachments: msaaImage., depthImage, swapchain imageViews");
  for (uint_fast8_t i = 0; i < swapchain.images.size(); i++) {
    std::array<VkImageView, 3> attachments{msaa_view,
                         depth_view,
                         swapchain.images[i].view};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .layers = 1};

    CE::vulkan_result(vkCreateFramebuffer,
              CE::BaseDevice::base_device->logical_device,
              &framebufferInfo,
              nullptr,
              &swapchain.framebuffers[i]);
  }
}

CE::BasePipelinesConfiguration::~BasePipelinesConfiguration() {
  if (BaseDevice::base_device) {
    Log::text(
        "{ === }", "destructing", this->pipeline_map.size(), "Pipelines Configuration");
    for (auto &[name, variant] : this->pipeline_map) {
      VkPipeline &pipelineObject = get_pipeline_object_by_name(name);
      vkDestroyPipeline(BaseDevice::base_device->logical_device, pipelineObject, nullptr);
    }
  }
}

void CE::BasePipelinesConfiguration::create_pipelines(VkRenderPass &render_pass,
                                                  const VkPipelineLayout &graphics_layout,
                                                  const VkPipelineLayout &compute_layout,
                                                  VkSampleCountFlagBits &msaa_samples) {
  if (this->pipeline_map.empty()) {
    throw std::runtime_error("\n!ERROR! No pipeline configurations defined.");
  }

  const auto pipelinesStart = std::chrono::high_resolution_clock::now();

  for (auto &[pipelineName, pipelineVariant] : this->pipeline_map) {
    const auto pipelineStart = std::chrono::high_resolution_clock::now();

    std::vector<std::string> shaders = get_pipeline_shaders_by_name(pipelineName);
    if (shaders.empty()) {
      throw std::runtime_error("\n!ERROR! Pipeline has no shaders: " + pipelineName);
    }

    const bool isCompute = std::any_of(
      shaders.begin(), shaders.end(), [](const std::string &shader) {
        return shader.ends_with("Comp");
      });

    if (!isCompute) {
      Log::text("{ === }", "Graphics Pipeline: ", pipelineName);
      std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
      bool tesselationEnabled = set_shader_stages(pipelineName, shaderStages);

      const auto &bindingDescription =
            std::get<CE::BasePipelinesConfiguration::Graphics>(pipelineVariant).vertex_bindings;
      const auto &attributesDescription =
          std::get<CE::BasePipelinesConfiguration::Graphics>(pipelineVariant)
              .vertex_attributes;
      uint32_t bindingsSize = static_cast<uint32_t>(bindingDescription.size());
      uint32_t attributeSize = static_cast<uint32_t>(attributesDescription.size());

      if (bindingsSize == 0 || attributeSize == 0) {
        throw std::runtime_error("\n!ERROR! Graphics pipeline has empty vertex "
                                 "bindings or attributes: " +
                                 pipelineName);
      }

      for (const auto &item : bindingDescription) {
        Log::text(Log::Style::char_leader,
                  "binding:",
                  item.binding,
                  item.inputRate ? "VK_VERTEX_INPUT_RATE_INSTANCE"
                                 : "VK_VERTEX_INPUT_RATE_VERTEX");
      }

      VkPipelineVertexInputStateCreateInfo vertexInput{CE::vertex_input_state_default};
      vertexInput.vertexBindingDescriptionCount = bindingsSize;
      vertexInput.vertexAttributeDescriptionCount = attributeSize;
      vertexInput.pVertexBindingDescriptions = bindingDescription.data();
      vertexInput.pVertexAttributeDescriptions = attributesDescription.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{
          CE::input_assembly_state_triangle_list};

        VkPipelineRasterizationStateCreateInfo rasterization{CE::rasterization_cull_back_bit};
      rasterization.depthBiasEnable = VK_FALSE;
      rasterization.depthBiasConstantFactor = 0.0f;
      rasterization.depthBiasSlopeFactor = 0.0f;
      rasterization.depthBiasClamp = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{CE::multisample_state_default};
      multisampling.rasterizationSamples = msaa_samples;
        VkPipelineDepthStencilStateCreateInfo depthStencil{CE::depth_stencil_state_default};

      static VkPipelineColorBlendAttachmentState colorBlendAttachment{
          CE::color_blend_attachment_state_false};
        VkPipelineColorBlendStateCreateInfo colorBlend{CE::color_blend_state_default};
      colorBlend.pAttachments = &colorBlendAttachment;

        VkPipelineViewportStateCreateInfo viewport{CE::viewport_state_default};
        VkPipelineDynamicStateCreateInfo dynamic{CE::dynamic_state_default};

      VkGraphicsPipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
          .stageCount = static_cast<uint32_t>(shaderStages.size()),
          .pStages = shaderStages.data(),
          .pVertexInputState = &vertexInput,
          .pInputAssemblyState = &inputAssembly,
          .pViewportState = &viewport,
          .pRasterizationState = &rasterization,
          .pMultisampleState = &multisampling,
          .pDepthStencilState = &depthStencil,
          .pColorBlendState = &colorBlend,
          .pDynamicState = &dynamic,
          .layout = graphics_layout,
          .renderPass = render_pass,
          .subpass = 0,
          .basePipelineHandle = VK_NULL_HANDLE};

      if (pipelineName.find("WireFrame") != std::string::npos) {
        rasterization.polygonMode = VK_POLYGON_MODE_LINE;
        rasterization.lineWidth = 1.05f;
        rasterization.depthBiasEnable = VK_FALSE;
        rasterization.cullMode = VK_CULL_MODE_NONE;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        colorBlendAttachment = CE::color_blend_attachment_state_average;
      }

      if (pipelineName == "Sky") {
        rasterization.cullMode = VK_CULL_MODE_NONE;
        rasterization.depthBiasEnable = VK_FALSE;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        colorBlendAttachment = CE::color_blend_attachment_state_false;
      }

      if (pipelineName == "TerrainBox") {
        rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization.depthBiasEnable = VK_TRUE;
        rasterization.depthBiasConstantFactor = 1.0f;
        rasterization.depthBiasSlopeFactor = 1.0f;
        rasterization.depthBiasClamp = 0.0f;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        colorBlendAttachment = CE::color_blend_attachment_state_false;
      }

      if (pipelineName == "CellsFollower") {
        rasterization.depthBiasEnable = VK_FALSE;
        rasterization.depthBiasConstantFactor = 0.0f;
        rasterization.depthBiasSlopeFactor = 0.0f;
        rasterization.depthBiasClamp = 0.0f;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
      }

      VkPipelineTessellationStateCreateInfo tessellationStateInfo{
          CE::tessellation_state_default};
      if (tesselationEnabled) {
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        if (pipelineName.find("WireFrame") == std::string::npos) {
          rasterization.polygonMode = VK_POLYGON_MODE_LINE;
          rasterization.lineWidth = 1.0f;
          colorBlendAttachment = CE::color_blend_attachment_state_multiply;
        }
        pipelineInfo.pTessellationState = &tessellationStateInfo;
      }

      CE::vulkan_result(vkCreateGraphicsPipelines,
            BaseDevice::base_device->logical_device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &get_pipeline_object_by_name(pipelineName));
      destroy_shader_modules();
    } else if (isCompute) {
      Log::text("{ === }", "Compute  Pipeline: ", pipelineName);

        const std::array<uint32_t, 3> &workGroups =
          get_work_groups_by_name(pipelineName);
      Log::text(Log::Style::char_leader,
                "workgroups",
                workGroups[0],
                workGroups[1],
                workGroups[2]);

      const std::string shaderToken = shaders[0];
      const std::string shaderModuleName =
          (shaderToken == "Comp") ? (pipelineName + shaderToken) : shaderToken;

      VkPipelineShaderStageCreateInfo shaderStage{create_shader_modules(
          VK_SHADER_STAGE_COMPUTE_BIT, shaderModuleName + ".spv")};

      VkComputePipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage = shaderStage,
          .layout = compute_layout};

      CE::vulkan_result(vkCreateComputePipelines,
            BaseDevice::base_device->logical_device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &get_pipeline_object_by_name(pipelineName));
      destroy_shader_modules();
    }

    const auto pipelineEnd = std::chrono::high_resolution_clock::now();
    const double pipelineMs =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            pipelineEnd - pipelineStart)
            .count();
    Log::text("{ PERF }", "Pipeline create", pipelineName, pipelineMs, "ms");
  }

  const auto pipelinesEnd = std::chrono::high_resolution_clock::now();
  const double totalMs =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          pipelinesEnd - pipelinesStart)
          .count();
  Log::text("{ PERF }", "All pipelines created in", totalMs, "ms");
}

bool CE::BasePipelinesConfiguration::set_shader_stages(
    const std::string &pipelineName,
    std::vector<VkPipelineShaderStageCreateInfo> &shaderStages) {
  std::vector<std::string> shaders = get_pipeline_shaders_by_name(pipelineName);
  std::string shaderName{};
  const std::array<std::string_view, 5> possibleStages = {"Vert", "Tesc", "Tese", "Frag"};
  const std::unordered_map<std::string_view, VkShaderStageFlagBits> shaderType = {
      {"Vert", VK_SHADER_STAGE_VERTEX_BIT},
      {"Tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
      {"Tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
      {"Frag", VK_SHADER_STAGE_FRAGMENT_BIT}};
  bool tesselationEnabled{false};

  for (uint32_t i = 0; i < shaders.size(); i++) {
    VkShaderStageFlagBits shaderStage{};

    if (shaderType.contains(shaders[i])) {
      shaderName = pipelineName + shaders[i];
      shaderStage = shaderType.at(shaders[i]);
    } else {
      shaderName = shaders[i];

      for (const std::string_view &stage : possibleStages) {
        size_t foundPosition = shaderName.find(stage);
        if (foundPosition != std::string_view::npos) {
          shaderStage = shaderType.at(stage);
          break;
        }
      }
    }
    if (!tesselationEnabled) {
      tesselationEnabled =
          shaderStage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ? true : false;
    }
    shaderStages.push_back(create_shader_modules(shaderStage, shaderName + ".spv"));
  }
  return tesselationEnabled;
}

std::vector<char>
CE::BasePipelinesConfiguration::read_shader_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("\n!ERROR! failed to open file!");
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

VkPipelineShaderStageCreateInfo
CE::BasePipelinesConfiguration::create_shader_modules(VkShaderStageFlagBits shaderStage,
                                                  std::string shaderName) {
  Log::text(Log::Style::char_leader, "Shader Module", shaderName);

  std::string shaderPath = resolve_shader_spv_path(this->shader_dir, shaderName);
  auto shaderCode = read_shader_file(shaderPath);
  VkShaderModule shaderModule{VK_NULL_HANDLE};

  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shaderCode.size(),
      .pCode = reinterpret_cast<const uint32_t *>(shaderCode.data())};

  CE::vulkan_result(vkCreateShaderModule,
                    BaseDevice::base_device->logical_device,
                    &createInfo,
                    nullptr,
                    &shaderModule);

  shader_modules.push_back(shaderModule);

  VkPipelineShaderStageCreateInfo shaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = shaderStage,
      .module = shaderModule,
      .pName = "main"};

  return shaderStageInfo;
}

void CE::BasePipelinesConfiguration::compile_shaders() {
  Log::text("{ GLSL }", "Compile Shaders");
  std::string systemCommand{};
  std::string pipelineName{};

  const std::unordered_map<std::string, std::string> stage_tokens{{"Comp", "comp"},
                                                                   {"Vert", "vert"},
                                                                   {"Tesc", "tesc"},
                                                                   {"Tese", "tese"},
                                                                   {"Frag", "frag"},
                                                                   {"Geom", "geom"}};

  const auto resolve_stage_extension =
      [&](const std::string &shader_name) -> std::pair<std::string, std::string> {
    const auto token = stage_tokens.find(shader_name);
    if (token != stage_tokens.end()) {
      return {pipelineName, token->second};
    }

    for (const auto &[token, extension] : stage_tokens) {
      if (shader_name.ends_with(token)) {
        return {shader_name.substr(0, shader_name.size() - token.size()), extension};
      }
    }
    return {"", ""};
  };

  for (const auto &[name, variant] : this->pipeline_map) {
    pipelineName = name;
    std::vector<std::string> shaders = get_pipeline_shaders_by_name(pipelineName);
    for (const auto &shader : shaders) {
      const auto [source_base, extension] = resolve_stage_extension(shader);
      if (source_base.empty() || extension.empty()) {
        continue;
      }

      std::string shaderSourcePath = this->shader_dir + source_base + "." + extension;
      std::string shaderOutputPath = shaderSourcePath + ".spv";
      bool shouldCompile = true;
      if (std::filesystem::exists(shaderOutputPath)) {
        const auto sourceWriteTime = std::filesystem::last_write_time(shaderSourcePath);
        const auto outputWriteTime = std::filesystem::last_write_time(shaderOutputPath);
        shouldCompile = sourceWriteTime > outputWriteTime;
      }

      if (!shouldCompile) {
        continue;
      }
      systemCommand = Lib::path(shaderSourcePath + " -o " + shaderOutputPath);
      system(systemCommand.c_str());
    }
  }
}

VkPipeline &CE::BasePipelinesConfiguration::get_pipeline_object_by_name(
    const std::string &name) {
  std::variant<Graphics, Compute> &variant = this->pipeline_map.at(name);
  if (std::holds_alternative<Graphics>(variant)) {
    return std::get<Graphics>(variant).pipeline;
  } else {
    return std::get<Compute>(variant).pipeline;
  }
}

void CE::BasePipelinesConfiguration::destroy_shader_modules() {
  for (uint_fast8_t i = 0; i < this->shader_modules.size(); i++) {
    vkDestroyShaderModule(BaseDevice::base_device->logical_device,
                          this->shader_modules[i],
                nullptr);
  }
  this->shader_modules.resize(0);
}

const std::vector<std::string> &
CE::BasePipelinesConfiguration::get_pipeline_shaders_by_name(const std::string &name) {
  std::variant<Graphics, Compute> &variant = this->pipeline_map.at(name);

  if (std::holds_alternative<Graphics>(variant)) {
    return std::get<Graphics>(variant).shaders;
  } else {
    return std::get<Compute>(variant).shaders;
  }
}

const std::array<uint32_t, 3> &
CE::BasePipelinesConfiguration::get_work_groups_by_name(const std::string &name) {
  std::variant<Graphics, Compute> &variant = this->pipeline_map.at(name);
  return std::get<CE::BasePipelinesConfiguration::Compute>(variant).work_groups;
};

void CE::BasePipelineLayout::create_layout(const VkDescriptorSetLayout &set_layout) {
  VkPipelineLayoutCreateInfo layout{CE::layout_default};
  layout.pSetLayouts = &set_layout;
  CE::vulkan_result(vkCreatePipelineLayout,
                    BaseDevice::base_device->logical_device,
                    &layout,
                    nullptr,
                    &this->layout);
}

void CE::BasePipelineLayout::create_layout(const VkDescriptorSetLayout &set_layout,
                                       const BasePushConstants &push_constants) {
  VkPushConstantRange constants{.stageFlags = push_constants.shader_stage,
                                .offset = push_constants.offset,
                                .size = push_constants.size};
  VkPipelineLayoutCreateInfo layout{CE::layout_default};
  layout.pSetLayouts = &set_layout;
  layout.pushConstantRangeCount = push_constants.count;
  layout.pPushConstantRanges = &constants;
  CE::vulkan_result(vkCreatePipelineLayout,
                    BaseDevice::base_device->logical_device,
                    &layout,
                    nullptr,
                    &this->layout);
}

CE::BasePipelineLayout::~BasePipelineLayout() {
  if (BaseDevice::base_device) {
    vkDestroyPipelineLayout(BaseDevice::base_device->logical_device, this->layout, nullptr);
  }
}

CE::BasePushConstants::BasePushConstants(VkShaderStageFlags stage,
                                 uint32_t data_size,
                                 uint32_t data_offset) {
  shader_stage = stage;

  size = (data_size % 4 == 0) ? data_size : ((data_size + 3) & ~3);
  if (size > 128) {
    size = 128;
  }
  offset = (data_offset % 4 == 0) ? data_offset : ((data_offset + 3) & ~3);
  count = 1;
  std::fill(data.begin(), data.end(), 0);

  if (size > data.size() * sizeof(uint64_t)) {
    throw std::runtime_error("Size exceeds the available space in the data array.");
  }
}

void CE::BasePushConstants::set_data(const uint64_t &value) {
  this->data = {value};
}

void CE::BasePushConstants::set_data(uint32_t value, float fraction) {
  std::fill(data.begin(), data.end(), 0);
  std::memcpy(data.data(), &value, sizeof(uint32_t));
  std::memcpy(reinterpret_cast<uint8_t *>(data.data()) + sizeof(uint32_t),
              &fraction,
              sizeof(float));
}

void CE::BasePushConstants::set_data(const uint64_t &value, float fraction) {
  std::fill(data.begin(), data.end(), 0);
  std::memcpy(data.data(), &value, sizeof(uint64_t));
  std::memcpy(reinterpret_cast<uint8_t *>(data.data()) + sizeof(uint64_t),
              &fraction,
              sizeof(float));
}
