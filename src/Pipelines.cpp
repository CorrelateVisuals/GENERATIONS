#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "CapitalEngine.h"
#include "Pipelines.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{},
      graphics{},
      render{resources.msaaImage.info.samples, mechanics.swapchain.imageFormat},
      _mechanics(mechanics) {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}

void Pipelines::setupPipelines(Resources& _resources) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ >>> }", "Setup Pipelines");

  // render.create(_resources.msaaImage.info.samples,
  //               _mechanics.swapchain.imageFormat);
  graphics.createGraphicsLayout(CE::Descriptor::setLayout);
  compute.createComputeLayout(CE::Descriptor::setLayout,
                              _resources.pushConstants);
  config.createPipelines(render.renderPass, graphics.layout, compute.layout,
                         _resources.msaaImage.info.samples);
}
