#pragma once

#include <array>
#include <cstdint>

#include <vulkan/vulkan.h>

namespace CE::RenderGUI {

struct StageStripConfig {
  bool enabled = true;
  uint32_t strip_height_px = 180;
  uint32_t padding_px = 8;
};

StageStripConfig get_stage_strip_config(const VkExtent2D &extent);
const std::array<const char *, 5>& get_stage_strip_labels();
bool is_stage_strip_enabled();

} // namespace CE::RenderGUI
