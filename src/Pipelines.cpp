#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "CapitalEngine.h"
#include "Pipelines.h"

#include <array>
#include <cstring>
#include <random>

Pipelines::Pipelines(VulkanMechanics& mechanics)
    : compute{}, graphics{}, _mechanics(mechanics) {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}

void Pipelines::setupPipelines(Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ === }", "Setup Pipelines");

  _resources.createDescriptorSetLayout();
  createRenderPass();

  createColorResources(_resources);
  createDepthResources(_resources);

  createGraphicsPipeline_Layout(_resources.descriptor);
  createGraphicsPipeline_Cells();
  createGraphicsPipeline_Landscape();
  createGraphicsPipeline_LandscapeWireframe();
  createGraphicsPipeline_Water();
  createGraphicsPipeline_Texture();

  createComputePipeline_Layout(_resources.descriptor, _resources.pushConstants);
  createComputePipeline_Engine();
  createComputePipeline_PostFX();
}

void Pipelines::createColorResources(Resources& _resources) {
  Log::text("{ []< }", "Color Resources ");

  VkFormat colorFormat = _mechanics.swapChain.imageFormat;

  _resources.createImage(
      _mechanics.swapChain.extent.width, _mechanics.swapChain.extent.height,
      graphics.msaa.samples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, graphics.msaa.image,
      graphics.msaa.imageMemory);
  graphics.msaa.imageView = _resources.createImageView(
      graphics.msaa.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Pipelines::createDepthResources(Resources& _resources) {
  Log::text("{ []< }", "Depth Resources ");
  VkFormat depthFormat = findDepthFormat();

  _resources.createImage(
      _mechanics.swapChain.extent.width, _mechanics.swapChain.extent.height,
      graphics.msaa.samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, graphics.depth.image,
      graphics.depth.imageMemory);
  graphics.depth.imageView = _resources.createImageView(
      graphics.depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Pipelines::createRenderPass() {
  Log::text("{ []< }", "Render Pass");
  Log::text(Log::Style::charLeader,
            "colorAttachment, depthAttachment, colorAttachmentResolve");

  VkAttachmentDescription colorAttachment{
      .format = _mechanics.swapChain.imageFormat,
      .samples = graphics.msaa.samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription depthAttachment{
      .format = findDepthFormat(),
      .samples = graphics.msaa.samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentDescription colorAttachmentResolve{
      .format = _mechanics.swapChain.imageFormat,
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

  _mechanics.result(vkCreateRenderPass, _mechanics.mainDevice.logical,
                    &renderPassInfo, nullptr, &graphics.renderPass);
}

void Pipelines::createGraphicsPipeline_Layout(
    const Resources::DescriptorSets& _descriptorSets) {
  VkPipelineLayoutCreateInfo graphicsLayout{layoutDefault};
  graphicsLayout.pSetLayouts = &_descriptorSets.setLayout;
  _mechanics.result(vkCreatePipelineLayout, _mechanics.mainDevice.logical,
                    &graphicsLayout, nullptr, &graphics.layout);
}

void Pipelines::createComputePipeline_Layout(
    const Resources::DescriptorSets& _descriptorSets,
    const Resources::PushConstants& _pushConstants) {
  VkPushConstantRange constants{.stageFlags = _pushConstants.shaderStage,
                                .offset = _pushConstants.offset,
                                .size = _pushConstants.size};
  VkPipelineLayoutCreateInfo computeLayout{layoutDefault};
  computeLayout.pSetLayouts = &_descriptorSets.setLayout;
  computeLayout.pushConstantRangeCount = _pushConstants.count;
  computeLayout.pPushConstantRanges = &constants;
  _mechanics.result(vkCreatePipelineLayout, _mechanics.mainDevice.logical,
                    &computeLayout, nullptr, &compute.layout);
}

void Pipelines::createGraphicsPipeline_Cells() {
  Log::text("{ === }", "Graphics Pipeline: Cells");

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "CellsVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "CellsFrag.spv")};
  static auto bindings = World::Cell::getBindingDescription();
  static auto attributes = World::Cell::getAttributeDescriptions();
  uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
  uint32_t attributeSize = static_cast<uint32_t>(attributes.size());

  VkPipelineVertexInputStateCreateInfo vertexInput{vertexInputStateDefault};
  vertexInput.vertexBindingDescriptionCount = bindingsSize;
  vertexInput.vertexAttributeDescriptionCount = attributeSize;
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      inputAssemblyStateTriangleList};

  VkPipelineRasterizationStateCreateInfo rasterization{
      rasterizationCullBackBit};
  VkPipelineMultisampleStateCreateInfo multisampling{multisampleStateDefault};
  multisampling.rasterizationSamples = graphics.msaa.samples;
  VkPipelineDepthStencilStateCreateInfo depthStencil{depthStencilStateDefault};

  static VkPipelineColorBlendAttachmentState colorBlendAttachment{
      colorBlendAttachmentStateFalse};
  VkPipelineColorBlendStateCreateInfo colorBlend{colorBlendStateDefault};
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{viewportStateDefault};
  VkPipelineDynamicStateCreateInfo dynamic{dynamicStateDefault};

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
      .renderPass = graphics.renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE};

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics.cells);
  destroyShaderModules(shaderModules);
}

void Pipelines::createGraphicsPipeline_Landscape() {
  Log::text("{ === }", "Graphics Pipeline: Landscape");

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "LandscapeVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "LandscapeFrag.spv")};
  static auto bindings = World::Landscape::getBindingDescription();
  static auto attributes = World::Landscape::getAttributeDescriptions();
  uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
  uint32_t attributeSize = static_cast<uint32_t>(attributes.size());

  VkPipelineVertexInputStateCreateInfo vertexInput{vertexInputStateDefault};
  vertexInput.vertexBindingDescriptionCount = bindingsSize;
  vertexInput.vertexAttributeDescriptionCount = attributeSize;
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      inputAssemblyStateTriangleList};
  VkPipelineRasterizationStateCreateInfo rasterization{
      rasterizationCullBackBit};
  VkPipelineMultisampleStateCreateInfo multisampling{multisampleStateDefault};
  multisampling.rasterizationSamples = graphics.msaa.samples;
  VkPipelineDepthStencilStateCreateInfo depthStencil{depthStencilStateDefault};

  static VkPipelineColorBlendAttachmentState colorBlendAttachment{
      colorBlendAttachmentStateFalse};
  VkPipelineColorBlendStateCreateInfo colorBlend{colorBlendStateDefault};
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{viewportStateDefault};
  VkPipelineDynamicStateCreateInfo dynamic{dynamicStateDefault};

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
      .renderPass = graphics.renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE};

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                    &graphics.landscape);
  destroyShaderModules(shaderModules);
}

void Pipelines::createGraphicsPipeline_LandscapeWireframe() {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "LandscapeVert.spv"),
      setShaderStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                     "LandscapeTesc.spv"),
      setShaderStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                     "LandscapeTese.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "LandscapeFrag.spv")};
  static auto bindings = World::Landscape::getBindingDescription();
  static auto attributes = World::Landscape::getAttributeDescriptions();
  uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
  uint32_t attributeSize = static_cast<uint32_t>(attributes.size());

  VkPipelineVertexInputStateCreateInfo vertexInput{vertexInputStateDefault};
  vertexInput.vertexBindingDescriptionCount = bindingsSize;
  vertexInput.vertexAttributeDescriptionCount = attributeSize;
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      inputAssemblyStateTriangleList};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
  VkPipelineTessellationStateCreateInfo tessellationStateInfo{
      tessellationStateDefault};

  VkPipelineRasterizationStateCreateInfo rasterization{
      rasterizationCullBackBit};
  rasterization.polygonMode = VK_POLYGON_MODE_LINE;
  rasterization.lineWidth = 5.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{multisampleStateDefault};
  multisampling.rasterizationSamples = graphics.msaa.samples;
  VkPipelineDepthStencilStateCreateInfo depthStencil{depthStencilStateDefault};

  static VkPipelineColorBlendAttachmentState colorBlendAttachment{
      colorBlendAttachmentStateMultiply};
  VkPipelineColorBlendStateCreateInfo colorBlend{colorBlendStateDefault};
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{viewportStateDefault};
  VkPipelineDynamicStateCreateInfo dynamic{dynamicStateDefault};

  VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pTessellationState = &tessellationStateInfo,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterization,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlend,
      .pDynamicState = &dynamic,
      .layout = graphics.layout,
      .renderPass = graphics.renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE};

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                    &graphics.landscapeWireframe);
  destroyShaderModules(shaderModules);
}

void Pipelines::createGraphicsPipeline_Water() {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "WaterVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "WaterFrag.spv")};

  static auto bindings = World::Rectangle::Vertex::getBindingDescription();
  static auto attributes = World::Rectangle::Vertex::getAttributeDescriptions();
  uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
  uint32_t attributeSize = static_cast<uint32_t>(attributes.size());

  VkPipelineVertexInputStateCreateInfo vertexInput{vertexInputStateDefault};
  vertexInput.vertexBindingDescriptionCount = bindingsSize;
  vertexInput.vertexAttributeDescriptionCount = attributeSize;
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      inputAssemblyStateTriangleList};
  VkPipelineRasterizationStateCreateInfo rasterization{
      rasterizationCullBackBit};
  VkPipelineMultisampleStateCreateInfo multisampling{multisampleStateDefault};
  multisampling.rasterizationSamples = graphics.msaa.samples;
  VkPipelineDepthStencilStateCreateInfo depthStencil{depthStencilStateDefault};

  static VkPipelineColorBlendAttachmentState colorBlendAttachment{
      colorBlendAttachmentStateFalse};
  colorBlendAttachment.blendEnable = VK_TRUE;

  VkPipelineColorBlendStateCreateInfo colorBlend{colorBlendStateDefault};
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{viewportStateDefault};
  VkPipelineDynamicStateCreateInfo dynamic{dynamicStateDefault};

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
      .renderPass = graphics.renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE};

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics.water);
  destroyShaderModules(shaderModules);
}

void Pipelines::createGraphicsPipeline_Texture() {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "TextureVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "TextureFrag.spv")};

  static auto bindings = World::Rectangle::Vertex::getBindingDescription();
  static auto attributes = World::Rectangle::Vertex::getAttributeDescriptions();
  uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
  uint32_t attributeSize = static_cast<uint32_t>(attributes.size());

  VkPipelineVertexInputStateCreateInfo vertexInput{vertexInputStateDefault};
  vertexInput.vertexBindingDescriptionCount = bindingsSize;
  vertexInput.vertexAttributeDescriptionCount = attributeSize;
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      inputAssemblyStateTriangleList};
  VkPipelineRasterizationStateCreateInfo rasterization{
      rasterizationCullBackBit};
  VkPipelineMultisampleStateCreateInfo multisampling{multisampleStateDefault};
  multisampling.rasterizationSamples = graphics.msaa.samples;
  VkPipelineDepthStencilStateCreateInfo depthStencil{depthStencilStateDefault};

  static VkPipelineColorBlendAttachmentState colorBlendAttachment{
      colorBlendAttachmentStateFalse};
  colorBlendAttachment.blendEnable = VK_TRUE;

  VkPipelineColorBlendStateCreateInfo colorBlend{colorBlendStateDefault};
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{viewportStateDefault};
  VkPipelineDynamicStateCreateInfo dynamic{dynamicStateDefault};

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
      .renderPass = graphics.renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE};

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                    &graphics.texture);
  destroyShaderModules(shaderModules);
}

void Pipelines::createGraphicsPipeline_Cube() {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "TextureVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "TextureFrag.spv")};

  static auto bindings = World::Rectangle::Vertex::getBindingDescription();
  static auto attributes = World::Rectangle::Vertex::getAttributeDescriptions();
  uint32_t bindingsSize = static_cast<uint32_t>(bindings.size());
  uint32_t attributeSize = static_cast<uint32_t>(attributes.size());

  VkPipelineVertexInputStateCreateInfo vertexInput{vertexInputStateDefault};
  vertexInput.vertexBindingDescriptionCount = bindingsSize;
  vertexInput.vertexAttributeDescriptionCount = attributeSize;
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      inputAssemblyStateTriangleList};
  VkPipelineRasterizationStateCreateInfo rasterization{
      rasterizationCullBackBit};
  VkPipelineMultisampleStateCreateInfo multisampling{multisampleStateDefault};
  multisampling.rasterizationSamples = graphics.msaa.samples;
  VkPipelineDepthStencilStateCreateInfo depthStencil{depthStencilStateDefault};

  static VkPipelineColorBlendAttachmentState colorBlendAttachment{
      colorBlendAttachmentStateFalse};
  colorBlendAttachment.blendEnable = VK_TRUE;

  VkPipelineColorBlendStateCreateInfo colorBlend{colorBlendStateDefault};
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport{viewportStateDefault};
  VkPipelineDynamicStateCreateInfo dynamic{dynamicStateDefault};

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
      .renderPass = graphics.renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE};

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                    &graphics.texture);
  destroyShaderModules(shaderModules);
}

VkFormat Pipelines::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling,
                                        VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(_mechanics.mainDevice.physical, format,
                                        &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("\n!ERROR! failed to find supported format!");
}

VkFormat Pipelines::findDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

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

void Pipelines::createComputePipeline_Engine() {
  Log::text("{ === }", "Compute Pipeline");

  VkPipelineShaderStageCreateInfo shaderStage{
      setShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, "EngineComp.spv")};

  VkComputePipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shaderStage,
      .layout = compute.layout};

  _mechanics.result(vkCreateComputePipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &compute.engine);

  destroyShaderModules(shaderModules);
}

void Pipelines::createComputePipeline_PostFX() {
  Log::text("{ === }", "Compute Engine Pipeline");

  VkPipelineShaderStageCreateInfo shaderStage{
      setShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, "PostFXComp.spv")};

  VkComputePipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shaderStage,
      .layout = compute.layout};

  _mechanics.result(vkCreateComputePipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &compute.postFX);

  destroyShaderModules(shaderModules);
}

VkShaderModule Pipelines::createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code.size(),
      .pCode = reinterpret_cast<const uint32_t*>(code.data())};

  VkShaderModule shaderModule{};

  _mechanics.result(vkCreateShaderModule, _mechanics.mainDevice.logical,
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
