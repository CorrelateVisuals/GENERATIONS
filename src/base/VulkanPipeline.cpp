#include "VulkanPipeline.h"
#include "VulkanPipelinePresets.h"
#include "VulkanUtils.h"

#include "../io/Library.h"
#include "../core/Log.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <stdexcept>

CE::RenderPass::~RenderPass() {
  Log::text("{ []< }", "destructing Render Pass");
  if (Device::baseDevice) {
    vkDestroyRenderPass(Device::baseDevice->logical, this->renderPass, nullptr);
  }
}

void CE::RenderPass::create(VkSampleCountFlagBits msaaImageSamples,
                            VkFormat swapchainImageFormat) {
  Log::text("{ []< }", "Render Pass");
  Log::text(Log::Style::charLeader,
            "colorAttachment, depthAttachment, colorAttachmentResolve");

  VkAttachmentDescription colorAttachment{
      .format = swapchainImageFormat,
      .samples = msaaImageSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription depthAttachment{
      .format = CE::Image::findDepthFormat(),
      .samples = msaaImageSamples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription colorAttachmentResolve{
      .format = swapchainImageFormat,
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

  VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  CE::VULKAN_RESULT(vkCreateRenderPass,
                    Device::baseDevice->logical,
                    &renderPassInfo,
                    nullptr,
                    &this->renderPass);
}

void CE::RenderPass::createFramebuffers(CE::Swapchain &swapchain,
                                        const VkImageView &msaaView,
                                        const VkImageView &depthView) const {
  Log::text("{ 101 }", "Frame Buffers:", swapchain.images.size());

  Log::text(Log::Style::charLeader,
            "attachments: msaaImage., depthImage, swapchain imageViews");
  for (uint_fast8_t i = 0; i < swapchain.images.size(); i++) {
    std::array<VkImageView, 3> attachments{msaaView, depthView, swapchain.images[i].view};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .layers = 1};

    CE::VULKAN_RESULT(vkCreateFramebuffer,
                      CE::Device::baseDevice->logical,
                      &framebufferInfo,
                      nullptr,
                      &swapchain.framebuffers[i]);
  }
}

CE::PipelinesConfiguration::~PipelinesConfiguration() {
  if (Device::baseDevice) {
    Log::text(
        "{ === }", "destructing", this->pipelineMap.size(), "Pipelines Configuration");
    for (auto &pipeline : this->pipelineMap) {
      VkPipeline &pipelineObject = getPipelineObjectByName(pipeline.first);
      vkDestroyPipeline(Device::baseDevice->logical, pipelineObject, nullptr);
    }
  }
}

void CE::PipelinesConfiguration::createPipelines(VkRenderPass &renderPass,
                                                 const VkPipelineLayout &graphicsLayout,
                                                 const VkPipelineLayout &computeLayout,
                                                 VkSampleCountFlagBits &msaaSamples) {
  if (this->pipelineMap.empty()) {
    throw std::runtime_error("\n!ERROR! No pipeline configurations defined.");
  }

  const auto pipelinesStart = std::chrono::high_resolution_clock::now();

  for (auto &entry : this->pipelineMap) {
    const auto pipelineStart = std::chrono::high_resolution_clock::now();
    const std::string pipelineName = entry.first;

    std::vector<std::string> shaders = getPipelineShadersByName(pipelineName);
    if (shaders.empty()) {
      throw std::runtime_error("\n!ERROR! Pipeline has no shaders: " + pipelineName);
    }

    bool isCompute = std::find(shaders.begin(), shaders.end(), "Comp") != shaders.end();

    if (!isCompute) {
      Log::text("{ === }", "Graphics Pipeline: ", pipelineName);
      std::variant<Graphics, Compute> &pipelineVariant = entry.second;

      std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
      bool tesselationEnabled = setShaderStages(pipelineName, shaderStages);

      const auto &bindingDescription =
          std::get<CE::PipelinesConfiguration::Graphics>(pipelineVariant).vertexBindings;
      const auto &attributesDescription =
          std::get<CE::PipelinesConfiguration::Graphics>(pipelineVariant)
              .vertexAttributes;
      uint32_t bindingsSize = static_cast<uint32_t>(bindingDescription.size());
      uint32_t attributeSize = static_cast<uint32_t>(attributesDescription.size());

      if (bindingsSize == 0 || attributeSize == 0) {
        throw std::runtime_error("\n!ERROR! Graphics pipeline has empty vertex "
                                 "bindings or attributes: " +
                                 pipelineName);
      }

      for (const auto &item : bindingDescription) {
        Log::text(Log::Style::charLeader,
                  "binding:",
                  item.binding,
                  item.inputRate ? "VK_VERTEX_INPUT_RATE_INSTANCE"
                                 : "VK_VERTEX_INPUT_RATE_VERTEX");
      }

      VkPipelineVertexInputStateCreateInfo vertexInput{CE::vertexInputStateDefault};
      vertexInput.vertexBindingDescriptionCount = bindingsSize;
      vertexInput.vertexAttributeDescriptionCount = attributeSize;
      vertexInput.pVertexBindingDescriptions = bindingDescription.data();
      vertexInput.pVertexAttributeDescriptions = attributesDescription.data();

      VkPipelineInputAssemblyStateCreateInfo inputAssembly{
          CE::inputAssemblyStateTriangleList};

      VkPipelineRasterizationStateCreateInfo rasterization{CE::rasterizationCullBackBit};

      VkPipelineMultisampleStateCreateInfo multisampling{CE::multisampleStateDefault};
      multisampling.rasterizationSamples = msaaSamples;
      VkPipelineDepthStencilStateCreateInfo depthStencil{CE::depthStencilStateDefault};

      static VkPipelineColorBlendAttachmentState colorBlendAttachment{
          CE::colorBlendAttachmentStateFalse};
      VkPipelineColorBlendStateCreateInfo colorBlend{CE::colorBlendStateDefault};
      colorBlend.pAttachments = &colorBlendAttachment;

      VkPipelineViewportStateCreateInfo viewport{CE::viewportStateDefault};
      VkPipelineDynamicStateCreateInfo dynamic{CE::dynamicStateDefault};

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
          .layout = graphicsLayout,
          .renderPass = renderPass,
          .subpass = 0,
          .basePipelineHandle = VK_NULL_HANDLE};

      VkPipelineTessellationStateCreateInfo tessellationStateInfo{
          CE::tessellationStateDefault};
      if (tesselationEnabled) {
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        rasterization.polygonMode = VK_POLYGON_MODE_LINE;
        rasterization.lineWidth = 5.0f;
        colorBlendAttachment = CE::colorBlendAttachmentStateMultiply;
        pipelineInfo.pTessellationState = &tessellationStateInfo;
      }

      CE::VULKAN_RESULT(vkCreateGraphicsPipelines,
                        Device::baseDevice->logical,
                        VK_NULL_HANDLE,
                        1,
                        &pipelineInfo,
                        nullptr,
                        &getPipelineObjectByName(pipelineName));
      destroyShaderModules();
    } else if (isCompute) {
      Log::text("{ === }", "Compute  Pipeline: ", pipelineName);

      const std::array<uint32_t, 3> &workGroups = getWorkGroupsByName(pipelineName);
      Log::text(Log::Style::charLeader,
                "workgroups",
                workGroups[0],
                workGroups[1],
                workGroups[2]);

      VkPipelineShaderStageCreateInfo shaderStage{createShaderModules(
          VK_SHADER_STAGE_COMPUTE_BIT, pipelineName + shaders[0] + ".spv")};

      VkComputePipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage = shaderStage,
          .layout = computeLayout};

      CE::VULKAN_RESULT(vkCreateComputePipelines,
                        Device::baseDevice->logical,
                        VK_NULL_HANDLE,
                        1,
                        &pipelineInfo,
                        nullptr,
                        &getPipelineObjectByName(pipelineName));
      destroyShaderModules();
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

bool CE::PipelinesConfiguration::setShaderStages(
    const std::string &pipelineName,
    std::vector<VkPipelineShaderStageCreateInfo> &shaderStages) {
  std::vector<std::string> shaders = getPipelineShadersByName(pipelineName);
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

    if (shaderType.find(shaders[i]) != shaderType.end()) {
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
    shaderStages.push_back(createShaderModules(shaderStage, shaderName + ".spv"));
  }
  return tesselationEnabled;
}

std::vector<char>
CE::PipelinesConfiguration::readShaderFile(const std::string &filename) {
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
CE::PipelinesConfiguration::createShaderModules(VkShaderStageFlagBits shaderStage,
                                                std::string shaderName) {
  Log::text(Log::Style::charLeader, "Shader Module", shaderName);

  std::string shaderPath = this->shaderDir + shaderName;
  auto shaderCode = readShaderFile(shaderPath);
  VkShaderModule shaderModule{VK_NULL_HANDLE};

  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shaderCode.size(),
      .pCode = reinterpret_cast<const uint32_t *>(shaderCode.data())};

  CE::VULKAN_RESULT(vkCreateShaderModule,
                    Device::baseDevice->logical,
                    &createInfo,
                    nullptr,
                    &shaderModule);

  shaderModules.push_back(shaderModule);

  VkPipelineShaderStageCreateInfo shaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = shaderStage,
      .module = shaderModule,
      .pName = "main"};

  return shaderStageInfo;
}

void CE::PipelinesConfiguration::compileShaders() {
  Log::text("{ GLSL }", "Compile Shaders");
  std::string systemCommand{};
  std::string shaderExtension{};
  std::string pipelineName{};

  for (const auto &entry : this->pipelineMap) {
    pipelineName = entry.first;
    std::vector<std::string> shaders = getPipelineShadersByName(pipelineName);
    for (const auto &shader : shaders) {
      if (shader == "Comp" || shader == "Vert" || shader == "Tesc" || shader == "Tese" ||
          shader == "Frag") {
        shaderExtension = Lib::upperToLowerCase(shader);
        std::string shaderSourcePath =
            this->shaderDir + pipelineName + "." + shaderExtension;
        std::string shaderOutputPath = this->shaderDir + pipelineName + shader + ".spv";
        std::ifstream outputFile(shaderOutputPath);
        if (outputFile.good()) {
          continue;
        }
        systemCommand = Lib::path(shaderSourcePath + " -o " + shaderOutputPath);
        system(systemCommand.c_str());
      }
    }
  }
}

VkPipeline &CE::PipelinesConfiguration::getPipelineObjectByName(const std::string &name) {
  std::variant<Graphics, Compute> &variant = this->pipelineMap.at(name);
  if (std::holds_alternative<Graphics>(variant)) {
    return std::get<Graphics>(variant).pipeline;
  } else {
    return std::get<Compute>(variant).pipeline;
  }
}

void CE::PipelinesConfiguration::destroyShaderModules() {
  for (uint_fast8_t i = 0; i < this->shaderModules.size(); i++) {
    vkDestroyShaderModule(Device::baseDevice->logical, this->shaderModules[i], nullptr);
  }
  this->shaderModules.resize(0);
}

const std::vector<std::string> &
CE::PipelinesConfiguration::getPipelineShadersByName(const std::string &name) {
  std::variant<Graphics, Compute> &variant = this->pipelineMap.at(name);

  if (std::holds_alternative<Graphics>(variant)) {
    return std::get<Graphics>(variant).shaders;
  } else {
    return std::get<Compute>(variant).shaders;
  }
}

const std::array<uint32_t, 3> &
CE::PipelinesConfiguration::getWorkGroupsByName(const std::string &name) {
  std::variant<Graphics, Compute> &variant = this->pipelineMap.at(name);
  return std::get<CE::PipelinesConfiguration::Compute>(variant).workGroups;
};

void CE::PipelineLayout::createLayout(const VkDescriptorSetLayout &setLayout) {
  VkPipelineLayoutCreateInfo layout{CE::layoutDefault};
  layout.pSetLayouts = &setLayout;
  CE::VULKAN_RESULT(vkCreatePipelineLayout,
                    Device::baseDevice->logical,
                    &layout,
                    nullptr,
                    &this->layout);
}

void CE::PipelineLayout::createLayout(const VkDescriptorSetLayout &setLayout,
                                      const PushConstants &_pushConstants) {
  VkPushConstantRange constants{.stageFlags = _pushConstants.shaderStage,
                                .offset = _pushConstants.offset,
                                .size = _pushConstants.size};
  VkPipelineLayoutCreateInfo layout{CE::layoutDefault};
  layout.pSetLayouts = &setLayout;
  layout.pushConstantRangeCount = _pushConstants.count;
  layout.pPushConstantRanges = &constants;
  CE::VULKAN_RESULT(vkCreatePipelineLayout,
                    Device::baseDevice->logical,
                    &layout,
                    nullptr,
                    &this->layout);
}

CE::PipelineLayout::~PipelineLayout() {
  if (Device::baseDevice) {
    vkDestroyPipelineLayout(Device::baseDevice->logical, this->layout, nullptr);
  }
}

CE::PushConstants::PushConstants(VkShaderStageFlags stage,
                                 uint32_t dataSize,
                                 uint32_t dataOffset) {
  shaderStage = stage;

  size = (dataSize % 4 == 0) ? dataSize : ((dataSize + 3) & ~3);
  if (size > 128) {
    size = 128;
  }
  offset = (dataOffset % 4 == 0) ? dataOffset : ((dataOffset + 3) & ~3);
  count = 1;
  std::fill(data.begin(), data.end(), 0);

  if (size > data.size() * sizeof(uint64_t)) {
    throw std::runtime_error("Size exceeds the available space in the data array.");
  }
}

void CE::PushConstants::setData(const uint64_t &data) {
  this->data = {data};
}
