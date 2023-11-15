#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "CapitalEngine.h"
#include "Pipelines.h"

Pipelines::Pipelines(VulkanMechanics& mechanics, Resources& resources)
    : compute{resources.pushConstants},
      graphics{},
      render{resources.msaaImage.info.samples, mechanics.swapchain.imageFormat},
      config{render.renderPass, graphics.layout, compute.layout,
             resources.msaaImage.info.samples} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
