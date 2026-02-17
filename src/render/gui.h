#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace CE::RenderGUI {

struct StageStripConfig {
  bool enabled = true;
  uint32_t strip_height_px = 72;
  uint32_t padding_px = 0;
};

struct StageStripTile {
  std::string label{};
  std::vector<std::string> pipelines{};
  int32_t preset_index = -1;  // -1 = not a preset, >=0 = preset number
};

StageStripConfig get_stage_strip_config(const VkExtent2D &extent);
const std::vector<StageStripTile> &get_stage_strip_tiles();
const StageStripTile *get_stage_strip_tile(size_t index);
int32_t find_stage_strip_tile_index(const VkExtent2D &extent,
                                    float normalized_x,
                                    float normalized_y);
bool is_stage_strip_enabled();
void log_stage_strip_tiles();

} // namespace CE::RenderGUI
