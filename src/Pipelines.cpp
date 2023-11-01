#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "CapitalEngine.h"
#include "Pipelines.h"

Pipelines::Pipelines(VulkanMechanics& mechanics)
    : compute{}, _mechanics(mechanics) {
  Log::text("{ === }", "constructing Pipelines");
#if _DEBUG
  config.compileShaders();
#endif
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");

  for (auto& pipeline : config.pipelineMap) {
    VkPipeline& pipelineObject = config.getPipelineObjectByName(pipeline.first);
    vkDestroyPipeline(_mechanics.mainDevice.logical, pipelineObject, nullptr);
  }
}

void Pipelines::setupPipelines(Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ >>> }", "Setup Pipelines");

  render.create(_resources.msaaImage.info.samples,
                _mechanics.swapchain.imageFormat);
  graphics.createGraphicsLayout(_resources.descriptor);
  compute.createComputeLayout(_resources.descriptor, _resources.pushConstants);
  config.createPipelines(render.renderPass, graphics.layout, compute.layout,
                         _resources.msaaImage.info.samples);
}
