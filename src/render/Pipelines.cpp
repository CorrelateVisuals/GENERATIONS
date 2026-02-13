#include <vulkan/vulkan.h>

#include "Mechanics.h"
#include "Pipelines.h"
#include "core/Log.h"
#include "Resources.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{resources.descriptorInterface, resources.pushConstant},
      graphics{ resources.descriptorInterface},
      render{mechanics.swapchain, resources.msaaImage,
             resources.depthImage.view},
      config{render.renderPass, graphics.layout, compute.layout,
             resources.msaaImage.info.samples, resources.world._grid.size} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
