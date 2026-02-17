#include "vulkan_mechanics/Mechanics.h"
#include "Pipelines.h"
#include "engine/Log.h"
#include "vulkan_resources/VulkanResources.h"

Pipelines::Pipelines(VulkanMechanics &mechanics, VulkanResources &resources)
    : compute{resources.descriptor_interface, resources.push_constant},
      graphics{resources.descriptor_interface}, render{mechanics.swapchain,
                                                       resources.msaa_image,
                                                       resources.depth_image.view},
      config{render.render_pass,
             graphics.layout,
             compute.layout,
             resources.msaa_image.info.samples,
              resources.world._grid.size,
              mechanics.swapchain.extent} {
  Log::text("{ === }", "constructing Pipelines");
}

Pipelines::~Pipelines() {
  Log::text("{ === }", "destructing Pipelines");
}
