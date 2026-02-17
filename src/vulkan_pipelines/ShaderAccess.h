#pragma once

// Command recording entry points for shader-driven passes.
// Exists to keep graphics/compute command encoding close to pipeline intent.
#include "vulkan_base/VulkanBaseSync.h"

namespace CE {

class ShaderAccess {
public:
  struct CommandResources : public CE::BaseCommandBuffers {
    CommandResources(const CE::BaseQueues::FamilyIndices &family_indices);
    void record_compute_command_buffer(VulkanResources &resources,
                                       Pipelines &pipelines,
                                       const uint32_t frame_index) override;
    void record_graphics_command_buffer(CE::BaseSwapchain &swapchain,
                                        VulkanResources &resources,
                                        Pipelines &pipelines,
                                        const uint32_t frame_index,
                                        const uint32_t image_index) override;
  };
};
} // namespace CE
