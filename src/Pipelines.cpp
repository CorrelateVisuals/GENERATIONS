#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "CapitalEngine.h"
#include "Pipelines.h"

#include <array>
#include <cstring>
#include <random>

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{}, _mechanics(mechanics), _resources(resources) {
  Log::text("{ === }", "constructing Pipelines");

  compileShaders(pipelineConfig);
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");

  for (auto& pipeline : pipelineConfig) {
    VkPipeline& pipelineObject = getVkPipelineObjectByName(pipeline.first);
    vkDestroyPipeline(_mechanics.mainDevice.logical, pipelineObject, nullptr);
  }

  vkDestroyPipelineLayout(_mechanics.mainDevice.logical, graphics.layout,
                          nullptr);
  vkDestroyPipelineLayout(_mechanics.mainDevice.logical, compute.layout,
                          nullptr);

  vkDestroyRenderPass(_mechanics.mainDevice.logical, renderPass.renderPass,
                      nullptr);
}

void Pipelines::setupPipelines(Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ >>> }", "Setup Pipelines");

  createRenderPass(_resources);
  createGraphicsPipeline_Layout(_resources.descriptor);
  createComputePipeline_Layout(_resources.descriptor, _resources.pushConstants);
  createPipelines(pipelineConfig, _resources.msaaImage.info.samples);
}

void Pipelines::createRenderPass(Resources& _resources) {
  Log::text("{ []< }", "Render Pass");
  Log::text(Log::Style::charLeader,
            "colorAttachment, depthAttachment, colorAttachmentResolve");

  VkAttachmentDescription colorAttachment{
      .format = _mechanics.swapchain.imageFormat,
      .samples = _resources.msaaImage.info.samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription depthAttachment{
      .format = CE::Image::findDepthFormat(),
      .samples = _resources.msaaImage.info.samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription colorAttachmentResolve{
      .format = _mechanics.swapchain.imageFormat,
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
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentReference colorAttachmentResolveRef{
      .attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
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

  CE::VULKAN_RESULT(vkCreateRenderPass, _mechanics.mainDevice.logical,
                    &renderPassInfo, nullptr, &renderPass.renderPass);
}

void Pipelines::createGraphicsPipeline_Layout(
    const Resources::DescriptorSets& _descriptorSets) {
  VkPipelineLayoutCreateInfo graphicsLayout{CE::layoutDefault};
  graphicsLayout.pSetLayouts = &_descriptorSets.setLayout;
  CE::VULKAN_RESULT(vkCreatePipelineLayout, _mechanics.mainDevice.logical,
                    &graphicsLayout, nullptr, &graphics.layout);
}

void Pipelines::createComputePipeline_Layout(
    const Resources::DescriptorSets& _descriptorSets,
    const Resources::PushConstants& _pushConstants) {
  VkPushConstantRange constants{.stageFlags = _pushConstants.shaderStage,
                                .offset = _pushConstants.offset,
                                .size = _pushConstants.size};
  VkPipelineLayoutCreateInfo computeLayout{CE::layoutDefault};
  computeLayout.pSetLayouts = &_descriptorSets.setLayout;
  computeLayout.pushConstantRangeCount = _pushConstants.count;
  computeLayout.pPushConstantRanges = &constants;
  CE::VULKAN_RESULT(vkCreatePipelineLayout, _mechanics.mainDevice.logical,
                    &computeLayout, nullptr, &compute.layout);
}

void Pipelines::createPipelines(PipelineConfiguration& pipelineConfig,
                                VkSampleCountFlagBits& msaaSamples) {
  for (auto& entry : pipelineConfig) {
    PIPELINE_TUPLE_UNPACKED = entry.second;

    bool isCompute = std::find(shaderExtensions.begin(), shaderExtensions.end(),
                               "Comp") != shaderExtensions.end();

    if (!isCompute) {
      Log::text("{ === }", "Graphics Pipeline: ", entry.first);
      std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

      VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
      for (size_t i = 0; i < shaderExtensions.size(); i++) {
        if (shaderExtensions[i] == "Vert") {
          shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (shaderExtensions[i] == "Frag") {
          shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (shaderExtensions[i] == "Comp") {
          shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
        } else if (shaderExtensions[i] == "Tesc") {
          shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        } else if (shaderExtensions[i] == "Tese") {
          shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        shaderStages.push_back(setShaderStage(
            shaderStage, entry.first + shaderExtensions[i] + ".spv"));
      }

      const auto& bindingDescription = binding.value()();
      const auto& attributesDescription = attribute.value()();
      uint32_t bindingsSize = static_cast<uint32_t>(bindingDescription.size());
      uint32_t attributeSize =
          static_cast<uint32_t>(attributesDescription.size());

      for (const auto& item : bindingDescription) {
        Log::text(Log::Style::charLeader, "binding:", item.binding,
                  item.inputRate ? "VK_VERTEX_INPUT_RATE_INSTANCE"
                                 : "VK_VERTEX_INPUT_RATE_VERTEX");
      }

      VkPipelineVertexInputStateCreateInfo vertexInput{
          CE::vertexInputStateDefault};
      vertexInput.vertexBindingDescriptionCount = bindingsSize;
      vertexInput.vertexAttributeDescriptionCount = attributeSize;
      vertexInput.pVertexBindingDescriptions = bindingDescription.data();
      vertexInput.pVertexAttributeDescriptions = attributesDescription.data();

      VkPipelineInputAssemblyStateCreateInfo inputAssembly{
          CE::inputAssemblyStateTriangleList};

      VkPipelineRasterizationStateCreateInfo rasterization{
          CE::rasterizationCullBackBit};

      VkPipelineMultisampleStateCreateInfo multisampling{
          CE::multisampleStateDefault};
      multisampling.rasterizationSamples = msaaSamples;
      VkPipelineDepthStencilStateCreateInfo depthStencil{
          CE::depthStencilStateDefault};

      static VkPipelineColorBlendAttachmentState colorBlendAttachment{
          CE::colorBlendAttachmentStateFalse};
      VkPipelineColorBlendStateCreateInfo colorBlend{
          CE::colorBlendStateDefault};
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
          .layout = graphics.layout,
          .renderPass = renderPass.renderPass,
          .subpass = 0,
          .basePipelineHandle = VK_NULL_HANDLE};

      CE::VULKAN_RESULT(vkCreateGraphicsPipelines,
                        _mechanics.mainDevice.logical, VK_NULL_HANDLE, 1,
                        &pipelineInfo, nullptr,
                        &getVkPipelineObjectByName(entry.first));
      destroyShaderModules(shaderModules);
    } else if (isCompute) {
      Log::text("{ === }", "Compute  Pipeline: ", entry.first);

      VkPipelineShaderStageCreateInfo shaderStage{
          setShaderStage(VK_SHADER_STAGE_COMPUTE_BIT,
                         entry.first + shaderExtensions[0] + ".spv")};

      VkComputePipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage = shaderStage,
          .layout = compute.layout};

      CE::VULKAN_RESULT(vkCreateComputePipelines, _mechanics.mainDevice.logical,
                        VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                        &getVkPipelineObjectByName(entry.first));
      destroyShaderModules(shaderModules);
    }
  }
}

// void Pipelines::createGraphicsPipeline_LandscapeWireframe(
//     VkSampleCountFlagBits& msaaSamples) {
//   std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
//       setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "LandscapeVert.spv"),
//       setShaderStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
//                      "LandscapeTesc.spv"),
//       setShaderStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
//                      "LandscapeTese.spv"),
//       setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "LandscapeFrag.spv")};
//   static auto bindings = World::Landscape::Vertex::getBindingDescription();
//   static auto attributes = World::Landscape::getAttributeDescriptions();
//   uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
//   uint32_t attributeSize = static_cast<uint32_t>(attributes.size());
//
//   VkPipelineVertexInputStateCreateInfo
//   vertexInput{CE::vertexInputStateDefault};
//   vertexInput.vertexBindingDescriptionCount = bindingsSize;
//   vertexInput.vertexAttributeDescriptionCount = attributeSize;
//   vertexInput.pVertexBindingDescriptions = bindings.data();
//   vertexInput.pVertexAttributeDescriptions = attributes.data();
//
//   VkPipelineInputAssemblyStateCreateInfo inputAssembly{
//       CE::inputAssemblyStateTriangleList};
//   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
//   VkPipelineTessellationStateCreateInfo tessellationStateInfo{
//       CE::tessellationStateDefault};
//
//   VkPipelineRasterizationStateCreateInfo rasterization{
//       CE::rasterizationCullBackBit};
//   rasterization.polygonMode = VK_POLYGON_MODE_LINE;
//   rasterization.lineWidth = 5.0f;
//
//   VkPipelineMultisampleStateCreateInfo multisampling{
//       CE::multisampleStateDefault};
//   multisampling.rasterizationSamples = msaaSamples;
//   VkPipelineDepthStencilStateCreateInfo depthStencil{
//       CE::depthStencilStateDefault};
//
//   static VkPipelineColorBlendAttachmentState colorBlendAttachment{
//       CE::colorBlendAttachmentStateMultiply};
//   VkPipelineColorBlendStateCreateInfo
//   colorBlend{CE::colorBlendStateDefault}; colorBlend.pAttachments =
//   &colorBlendAttachment;
//
//   VkPipelineViewportStateCreateInfo viewport{CE::viewportStateDefault};
//   VkPipelineDynamicStateCreateInfo dynamic{CE::dynamicStateDefault};
//
//   VkGraphicsPipelineCreateInfo pipelineInfo{
//       .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
//       .stageCount = static_cast<uint32_t>(shaderStages.size()),
//       .pStages = shaderStages.data(),
//       .pVertexInputState = &vertexInput,
//       .pInputAssemblyState = &inputAssembly,
//       .pTessellationState = &tessellationStateInfo,
//       .pViewportState = &viewport,
//       .pRasterizationState = &rasterization,
//       .pMultisampleState = &multisampling,
//       .pDepthStencilState = &depthStencil,
//       .pColorBlendState = &colorBlend,
//       .pDynamicState = &dynamic,
//       .layout = graphics.layout,
//       .renderPass = renderPass.renderPass,
//       .subpass = 0,
//       .basePipelineHandle = VK_NULL_HANDLE};
//
//   CE::VULKAN_RESULT(vkCreateGraphicsPipelines,
//   _mechanics.mainDevice.logical,
//                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
//                    &graphics.landscapeWireframe);
//   destroyShaderModules(shaderModules);
// }

bool Pipelines::hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkPipelineShaderStageCreateInfo Pipelines::setShaderStage(
    VkShaderStageFlagBits shaderStage,
    std::string shaderName) {
  Log::text(Log::Style::charLeader, "Shader Module", shaderName);

  std::string shaderPath = shaderDir + shaderName;
  auto shaderCode = readShaderFile(shaderPath);
  VkShaderModule shaderModule = createShaderModule(shaderCode);
  shaderModules.push_back(shaderModule);

  VkPipelineShaderStageCreateInfo shaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = shaderStage,
      .module = shaderModule,
      .pName = "main"};

  return shaderStageInfo;
}

void Pipelines::compileShaders(const auto& pipelineConfig) {
  Log::text("{ GLSL }", "Compile Shaders");
  std::string systemCommand = "";
  std::string shaderExtension = "";
  std::string pipelineName = "";

  for (const auto& pipeline : pipelineConfig) {
    pipelineName = pipeline.first;
    PIPELINE_TUPLE_UNPACKED = pipeline.second;

    for (const auto& shader : shaderExtensions) {
      shaderExtension = Lib::upperToLowerCase(shader);
      systemCommand =
          Lib::path(shaderDir + pipelineName + "." + shaderExtension + " -o " +
                    shaderDir + pipelineName + shader + ".spv");
      system(systemCommand.c_str());
    }
  }
}

std::vector<char> Pipelines::readShaderFile(const std::string& filename) {
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

VkShaderModule Pipelines::createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code.size(),
      .pCode = reinterpret_cast<const uint32_t*>(code.data())};

  VkShaderModule shaderModule{};

  CE::VULKAN_RESULT(vkCreateShaderModule, _mechanics.mainDevice.logical,
                    &createInfo, nullptr, &shaderModule);

  return shaderModule;
}

void Pipelines::destroyShaderModules(
    std::vector<VkShaderModule>& shaderModules) {
  for (size_t i = 0; i < shaderModules.size(); i++) {
    vkDestroyShaderModule(_mechanics.mainDevice.logical, shaderModules[i],
                          nullptr);
  }
  shaderModules.resize(0);
};
