#include <vulkan/vulkan.h>

#include "CapitalEngine.h"
#include "Pipelines.h"
#include "Resources.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{resources.pushConstants},
      graphics{},
      render{mechanics.swapchain, resources.msaaImage,
             resources.depthImage.view},
      config{render.renderPass, graphics.layout, compute.layout,
             resources.msaaImage.info.samples, resources.world.grid.size} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
