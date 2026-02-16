#include <vulkan/vulkan.h>

#include "CapitalEngine.h"
#include "Pipelines.h"
#include "Resources.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{resources.descriptorInterface, resources.pushConstant},
      graphics{resources.descriptorInterface},
      render{mechanics.swapchain, resources.msaaImage,
             resources.depthImage.view},
      config{render.renderPass, graphics.layout, compute.layout,
             resources.msaaImage.info.samples} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
