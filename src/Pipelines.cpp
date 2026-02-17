#include "render/Mechanics.h"
#include "Pipelines.h"
#include "core/Log.h"
#include "Resources.h"

Pipelines::Pipelines(VulkanMechanics &mechanics, Resources &resources)
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
