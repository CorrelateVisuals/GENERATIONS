#pragma once

// Top-level runtime orchestrator.
// Exists to coordinate mechanics, resources, pipelines, and per-frame execution.
#include "vulkan_mechanics/Mechanics.h"
#include "vulkan_pipelines/FrameContext.h"
#include "vulkan_pipelines/Pipelines.h"
#include "vulkan_resources/VulkanResources.h"

#include <memory>
#include <string>

class CapitalEngine {
public:
  CapitalEngine();
  CapitalEngine(const CapitalEngine &) = delete;
  CapitalEngine &operator=(const CapitalEngine &) = delete;
  CapitalEngine(CapitalEngine &&) = delete;
  CapitalEngine &operator=(CapitalEngine &&) = delete;
  ~CapitalEngine();

  void main_loop();

  // New methods for frame time and FPS
  float getFrameTime() const;
  float getFPS() const;

private:
  VulkanMechanics mechanics;
  std::unique_ptr<VulkanResources> resources;
  std::unique_ptr<Pipelines> pipelines;
  std::unique_ptr<FrameContext> frame_context;

  uint32_t last_presented_image_index{0};
  uint32_t last_submitted_frame_index{0};

  void recreate_swapchain();
  void draw_frame();
  void take_screenshot(const std::string &tag = "");
};
