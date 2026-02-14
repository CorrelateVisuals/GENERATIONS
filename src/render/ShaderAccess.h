#pragma once
#include "base/VulkanSync.h"
#include <functional>

namespace CE {

class ShaderAccess {
public:
  struct CommandResources : public CE::CommandBuffers {
    CommandResources(const CE::queues::family_indices &family_indices);
    void record_compute_command_buffer(Resources &resources,
                                       Pipelines &pipelines,
                                       const uint32_t frame_index) override;
    void record_graphics_command_buffer(CE::Swapchain &swapchain,
                                        Resources &resources,
                                        Pipelines &pipelines,
                                        const uint32_t frame_index,
                                        const uint32_t image_index) override;
    
    // Set a callback to be called before ending the render pass
    // This allows app layer to add additional rendering (e.g., ImGui)
    std::function<void(VkCommandBuffer)> pre_render_pass_end_callback;
  };
};
} // namespace CE
