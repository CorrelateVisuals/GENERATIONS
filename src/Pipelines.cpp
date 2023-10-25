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
  config.createPipelines(renderPass.renderPass, graphics.layout, compute.layout,
                         _resources.msaaImage.info.samples);
}
