#pragma once
#include "vulkan_base/VulkanSync.h"

namespace CE {

class ShaderAccess {
public:
  struct CommandResources : public CE::CommandBuffers {
    CommandResources(const CE::Queues::FamilyIndices &family_indices);
    void record_compute_command_buffer(Resources &resources,
                                       Pipelines &pipelines,
                                       const uint32_t frame_index) override;
    void record_graphics_command_buffer(CE::Swapchain &swapchain,
                                        Resources &resources,
                                        Pipelines &pipelines,
                                        const uint32_t frame_index,
                                        const uint32_t image_index) override;
  };
};
} // namespace CE
