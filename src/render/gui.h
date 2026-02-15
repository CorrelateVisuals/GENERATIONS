#pragma once

#include <array>

#include <vulkan/vulkan.h>

namespace CE::RenderGUI {

struct StageStripConfig {
  bool enabled = true;
  uint32_t strip_height_px = 180;
  uint32_t padding_px = 8;
};

StageStripConfig get_stage_strip_config(const VkExtent2D &extent);
std::array<const char *, 5> get_stage_strip_labels();

} // namespace CE::RenderGUI
