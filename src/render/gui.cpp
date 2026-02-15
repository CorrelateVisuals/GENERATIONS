#include "gui.h"

#include "core/RuntimeConfig.h"

#include <algorithm>
#include <cstdlib>

namespace CE::RenderGUI {

// Cache for stage strip configuration
struct ConfigCache {
  bool initialized = false;
  bool enabled = true;
  int32_t custom_height = -1;
  int32_t custom_padding = -1;
  
  void init_once() {
    if (initialized) return;
    initialized = true;
    
    const char *enabled_raw = std::getenv("CE_RENDER_STAGE_STRIP");
    enabled = enabled_raw ? CE::Runtime::env_truthy(enabled_raw) : true;
    
    if (const char *height_raw = std::getenv("CE_RENDER_STAGE_STRIP_HEIGHT")) {
      char *end = nullptr;
      const long parsed = std::strtol(height_raw, &end, 10);
      if (end != height_raw && *end == '\0' && parsed > 0) {
        custom_height = static_cast<int32_t>(parsed);
      }
    }
    
    if (const char *padding_raw = std::getenv("CE_RENDER_STAGE_STRIP_PADDING")) {
      char *end = nullptr;
      const long parsed = std::strtol(padding_raw, &end, 10);
      if (end != padding_raw && *end == '\0' && parsed >= 0) {
        custom_padding = static_cast<int32_t>(parsed);
      }
    }
  }
};

static ConfigCache s_config_cache;

StageStripConfig get_stage_strip_config(const VkExtent2D &extent) {
  s_config_cache.init_once();
  
  StageStripConfig config{};
  config.enabled = s_config_cache.enabled;
  
  const uint32_t max_reasonable_height = std::max<uint32_t>(extent.height / 2, 1);
  
  if (s_config_cache.custom_height > 0) {
    config.strip_height_px = std::clamp<uint32_t>(
        static_cast<uint32_t>(s_config_cache.custom_height), 32, max_reasonable_height);
  } else {
    config.strip_height_px = std::clamp<uint32_t>(extent.height / 5, 80, max_reasonable_height);
  }
  
  if (s_config_cache.custom_padding >= 0) {
    config.padding_px = std::clamp<uint32_t>(
        static_cast<uint32_t>(s_config_cache.custom_padding), 0, 64);
  }
  
  return config;
}

const std::array<const char *, 5>& get_stage_strip_labels() {
  static const std::array<const char *, 5> labels = 
      {"LandscapeDebug", "LandscapeStage1", "LandscapeStage2", "Landscape", "Full"};
  return labels;
}

bool is_stage_strip_enabled() {
  s_config_cache.init_once();
  return s_config_cache.enabled;
}

} // namespace CE::RenderGUI
