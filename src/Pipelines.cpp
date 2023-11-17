#include <vulkan/vulkan.h>

#include "CapitalEngine.h"
#include "Pipelines.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{resources.pushConstants},
      graphics{},
      render{resources.msaaImage.info.samples, mechanics.swapchain.imageFormat,
             mechanics.swapchain, resources.msaaImage.view,
             resources.depthImage.view},
      config{render.renderPass, graphics.layout, compute.layout,
             resources.msaaImage.info.samples} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
