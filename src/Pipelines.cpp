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

void Pipelines::createPipelines(Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ === }", "creating Pipelines");

  _resources.createDescriptorSetLayout();
  createRenderPass();
  createGraphicsPipelines(_resources.descriptor.setLayout);
  createComputePipeline(_resources.descriptor.setLayout,
                        _resources.pushConstants);
  createColorResources(_resources);
  createDepthResources(_resources);
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

void Pipelines::createGraphicsPipelines(
    const VkDescriptorSetLayout& descriptorSetLayout) {
  Log::text("{ === }", "Graphics Pipelines");

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "CellsVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "CellsFrag.spv")};
  static auto bindings =
      World::getCellBindingDescriptions(VK_VERTEX_INPUT_RATE_INSTANCE);
  static auto attributes = World::getCellAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInput = vertexInputStateDefault;
  vertexInput.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindings.size());
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly =
      inputAssemblyStateTriangleList;

  VkPipelineRasterizationStateCreateInfo rasterization = rasterizationDefault;

  VkPipelineMultisampleStateCreateInfo multisample = multisampleStateDefault;
  multisample.rasterizationSamples = graphics.msaa.samples;

  VkPipelineDepthStencilStateCreateInfo depthStencil = depthStencilStateDefault;

  static VkPipelineColorBlendAttachmentState colorBlendAttachment =
      colorBlendAttachmentStateFalse;
  VkPipelineColorBlendStateCreateInfo colorBlend = colorBlendStateDefault;
  colorBlend.pAttachments = &colorBlendAttachment;

  VkPipelineViewportStateCreateInfo viewport = viewportStateDefault;
  VkPipelineDynamicStateCreateInfo dynamic = dynamicStateDefault;

  VkPipelineLayoutCreateInfo layout = layoutDefault;
  layout.pSetLayouts = &descriptorSetLayout;

  _mechanics.result(vkCreatePipelineLayout, _mechanics.mainDevice.logical,
                    &layout, nullptr, &graphics.layout);

  VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterization,
      .pMultisampleState = &multisample,
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

  // Tiles pipeline
  shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "TilesVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "TilesFrag.spv")};
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics.tiles);
  destroyShaderModules(shaderModules);

  // Water pipeline
  shaderStages = {
      setShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "WaterVert.spv"),
      setShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "WaterFrag.spv")};
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  vertexInput = vertexInputStateDefault;
  pipelineInfo.pVertexInputState = &vertexInput;

  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlend.pAttachments = &colorBlendAttachment;
  pipelineInfo.pColorBlendState = &colorBlend;

  _mechanics.result(vkCreateGraphicsPipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics.water);
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

void Pipelines::createComputePipeline(
    const VkDescriptorSetLayout& descriptorSetLayout,
    const Resources::PushConstants& pushConstants) {
  Log::text("{ === }", "Compute Pipeline");

  VkPipelineShaderStageCreateInfo shaderStage{
      setShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, "EngineComp.spv")};

  VkPushConstantRange constants = Pipelines::pushConstantRange(
      pushConstants.shaderStage, pushConstants.offset, pushConstants.size);

  VkPipelineLayoutCreateInfo pipelineState = Pipelines::layoutState(
      descriptorSetLayout, pushConstants.count, constants);

  _mechanics.result(vkCreatePipelineLayout, _mechanics.mainDevice.logical,
                    &pipelineState, nullptr, &compute.layout);

  VkComputePipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shaderStage,
      .layout = compute.layout};

  _mechanics.result(vkCreateComputePipelines, _mechanics.mainDevice.logical,
                    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &compute.engine);

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

VkPipelineInputAssemblyStateCreateInfo Pipelines::inputAssemblyState(
    const VkPrimitiveTopology& topology) {
  VkPipelineInputAssemblyStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = topology,
      .primitiveRestartEnable = VK_FALSE};
  return createInfo;
}

VkPipelineVertexInputStateCreateInfo Pipelines::vertexInputState() {
  VkPipelineVertexInputStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = nullptr,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = nullptr};
  return createInfo;
}

VkPipelineVertexInputStateCreateInfo Pipelines::vertexInputState(
    const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>&
        attributeDescriptions) {
  VkPipelineVertexInputStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount =
          static_cast<uint32_t>(bindingDescriptions.size()),
      .pVertexBindingDescriptions = bindingDescriptions.data(),
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions = attributeDescriptions.data()};
  return createInfo;
}

VkPipelineDynamicStateCreateInfo Pipelines::dynamicState() {
  static std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data()};
  return createInfo;
}

VkPipelineViewportStateCreateInfo Pipelines::viewportState() {
  VkPipelineViewportStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};
  return createInfo;
}

VkPipelineLayoutCreateInfo Pipelines::layoutState(
    const VkDescriptorSetLayout& descriptorSetLayout) {
  VkPipelineLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayout};
  return createInfo;
}

VkPipelineLayoutCreateInfo Pipelines::layoutState(
    const VkDescriptorSetLayout& descriptorSetLayout,
    const uint32_t& pushConstantCount,
    const VkPushConstantRange& pushConstants) {
  VkPipelineLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstants};
  return createInfo;
}

VkPipelineMultisampleStateCreateInfo Pipelines::multisampleState(
    uint32_t enable,
    const VkSampleCountFlagBits& sampleCount) {
  VkPipelineMultisampleStateCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = sampleCount,
      .sampleShadingEnable = enable,
      .minSampleShading = 1.0f};
  return createInfo;
}

VkPushConstantRange Pipelines::pushConstantRange(
    const VkShaderStageFlags& flags,
    uint32_t offset,
    uint32_t size) {
  VkPushConstantRange info{.stageFlags = flags, .offset = offset, .size = size};
  return info;
}
