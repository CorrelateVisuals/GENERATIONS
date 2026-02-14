#pragma once
#include "render/Mechanics.h"
#include "render/Pipelines.h"
#include "render/Resources.h"
#include "app/ImGuiUI.h"

class CapitalEngine {
public:
  CapitalEngine();
  ~CapitalEngine();

  void main_loop();

private:
  VulkanMechanics mechanics;
  Resources resources;
  Pipelines pipelines;
  CE::ImGuiUI imgui_ui;

  uint32_t last_presented_image_index{0};
  uint32_t last_submitted_frame_index{0};

  void draw_frame();
  void take_screenshot();
};
