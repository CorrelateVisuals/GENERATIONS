#pragma once
#include "render/Mechanics.h"
#include "render/Pipelines.h"
#include "render/Resources.h"

#include <memory>

class CapitalEngine {
public:
  CapitalEngine();
  CapitalEngine(const CapitalEngine &) = delete;
  CapitalEngine &operator=(const CapitalEngine &) = delete;
  CapitalEngine(CapitalEngine &&) = delete;
  CapitalEngine &operator=(CapitalEngine &&) = delete;
  ~CapitalEngine();

  void main_loop();

private:
  VulkanMechanics mechanics;
  std::unique_ptr<Resources> resources;
  std::unique_ptr<Pipelines> pipelines;

  uint32_t last_presented_image_index{0};
  uint32_t last_submitted_frame_index{0};

  void draw_frame();
  void take_screenshot();
};
