#include <vulkan/vulkan.h>

#include "CapitalEngine.h"
#include "Pipelines.h"
#include "Resources.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{resources._pushConstant},
      graphics{},
      render{mechanics.swapchain, resources._msaaImage,
             resources._depthImage.view},
      config{render.renderPass, graphics.layout, compute.layout,
             resources._msaaImage.info.samples, resources._world._grid.size} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
