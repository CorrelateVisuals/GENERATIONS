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
  config.compileShaders();
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");

  for (auto& pipeline : config.pipelineMap) {
    VkPipeline& pipelineObject = config.getPipelineObjectByName(pipeline.first);
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

  renderPass.create(_resources.msaaImage.info.samples,
                    _mechanics.swapchain.imageFormat);
  graphics.createGraphicsLayout(_resources.descriptor);
  compute.createComputeLayout(_resources.descriptor, _resources.pushConstants);
  createPipelines(config.pipelineMap, _resources.msaaImage.info.samples);
}

void Pipelines::createPipelines(
    std::unordered_map<std::string,
                       std::variant<Configuration::Graphics,
                                    Configuration::Compute>>& pipelineMap,
    VkSampleCountFlagBits& msaaSamples) {
  for (auto& entry : pipelineMap) {
    const std::string pipelineName = entry.first;

    std::vector<std::string> shaders =
        config.getPipelineShadersByName(pipelineName);
    bool isCompute =
        std::find(shaders.begin(), shaders.end(), "Comp") != shaders.end();

    if (!isCompute) {
      Log::text("{ === }", "Graphics Pipeline: ", entry.first);
      std::variant<Configuration::Graphics, Configuration::Compute>& variant =
          pipelineMap[pipelineName];

      std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

      VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
      for (size_t i = 0; i < shaders.size(); i++) {
        if (shaders[i] == "Vert") {
          shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (shaders[i] == "Frag") {
          shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (shaders[i] == "Comp") {
          shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
        } else if (shaders[i] == "Tesc") {
          shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        } else if (shaders[i] == "Tese") {
          shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        shaderStages.push_back(config.createShaderModules(
            shaderStage, entry.first + shaders[i] + ".spv"));
      }

      const auto& bindingDescription =
          std::get<CE::Pipelines::Graphics>(variant).vertexBindings;
      const auto& attributesDescription =
          std::get<CE::Pipelines::Graphics>(variant).vertexAttributes;
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
                        &config.getPipelineObjectByName(pipelineName));
      config.destroyShaderModules();
    } else if (isCompute) {
      Log::text("{ === }", "Compute  Pipeline: ", entry.first);

      VkPipelineShaderStageCreateInfo shaderStage{config.createShaderModules(
          VK_SHADER_STAGE_COMPUTE_BIT, entry.first + shaders[0] + ".spv")};

      VkComputePipelineCreateInfo pipelineInfo{
          .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage = shaderStage,
          .layout = compute.layout};

      CE::VULKAN_RESULT(vkCreateComputePipelines, _mechanics.mainDevice.logical,
                        VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                        &config.getPipelineObjectByName(pipelineName));
      config.destroyShaderModules();
    }
  }
}
