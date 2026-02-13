#pragma once
#include "base/VulkanSync.h"

namespace CE {

class ShaderAccess {
 public:
  struct CommandResources : public CE::CommandBuffers {
    CommandResources(const CE::Queues::FamilyIndices& familyIndices);
    void recordComputeCommandBuffer(Resources& resources,
                                    Pipelines& pipelines,
                                    const uint32_t imageIndex) override;
    void recordGraphicsCommandBuffer(CE::Swapchain& swapchain,
                                     Resources& resources,
                                     Pipelines& pipelines,
                                     const uint32_t imageIndex) override;
  };
};
}  // namespace CE
