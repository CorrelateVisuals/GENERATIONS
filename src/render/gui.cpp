#include "gui.h"

#include "core/RuntimeConfig.h"
#include "core/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>

namespace CE::RenderGUI {

namespace {

struct StageStripCache {
  bool initialized = false;
  bool enabled = true;
  int32_t custom_height = -1;
  int32_t custom_padding = -1;
  std::vector<StageStripTile> tiles{};
};

StageStripCache g_stage_strip_cache{};
std::once_flag g_stage_strip_once;

std::string trim(std::string value) {
  const auto is_space = [](const unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  };

  while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

std::vector<std::string> split_trimmed(const std::string &raw, const char delimiter) {
  std::vector<std::string> values{};
  std::stringstream stream(raw);
  std::string token{};
  while (std::getline(stream, token, delimiter)) {
    token = trim(token);
    if (!token.empty()) {
      values.push_back(token);
    }
  }
  return values;
}

const std::vector<StageStripTile> &default_tiles() {
  static std::vector<StageStripTile> tiles{};
  if (!tiles.empty()) {
    return tiles;
  }

  // Add preset tiles first
  tiles.push_back(StageStripTile{.label = "Preset 2", .pipelines = {}, .preset_index = 2});
  tiles.push_back(StageStripTile{.label = "Preset 3", .pipelines = {}, .preset_index = 3});

  const auto &definitions = CE::Runtime::get_pipeline_definitions();
  std::vector<std::string> graphics_stage_names{};
  graphics_stage_names.reserve(definitions.size());
  for (const auto &[pipeline_name, definition] : definitions) {
    if (definition.is_compute) {
      continue;
    }
    if (!CE::Runtime::get_graphics_draw_op(pipeline_name)) {
      continue;
    }
    graphics_stage_names.push_back(pipeline_name);
  }
  std::sort(graphics_stage_names.begin(), graphics_stage_names.end());

  const std::vector<std::string> preferred_order = {
      "LandscapeDebug",
      "LandscapeStage1",
      "LandscapeStage2",
      "LandscapeNormals",
      "Sky",
      "Landscape",
      "TerrainBox",
      "CellsFollower",
      "Cells",
  };

  std::set<std::string> used_names{};
  std::vector<std::string> ordered_stages{};
  ordered_stages.reserve(graphics_stage_names.size());

  for (const std::string &name : preferred_order) {
    const bool exists = std::find(graphics_stage_names.begin(), graphics_stage_names.end(), name) !=
                        graphics_stage_names.end();
    if (exists) {
      ordered_stages.push_back(name);
      used_names.insert(name);
    }
  }

  for (const std::string &name : graphics_stage_names) {
    if (used_names.insert(name).second) {
      ordered_stages.push_back(name);
    }
  }

  std::vector<std::string> current_graphics{};
  if (const CE::Runtime::RenderGraph *graph = CE::Runtime::get_render_graph()) {
    for (const CE::Runtime::RenderNode &node : graph->nodes) {
      if (node.stage == CE::Runtime::RenderStage::Graphics) {
        current_graphics.push_back(node.pipeline);
      }
    }
  }
  if (!current_graphics.empty()) {
    tiles.push_back(StageStripTile{.label = "Current", .pipelines = current_graphics});
  }

  for (const std::string &stage : ordered_stages) {
    tiles.push_back(StageStripTile{.label = stage, .pipelines = {stage}});
  }

  const bool has_cells_follower =
      std::find(ordered_stages.begin(), ordered_stages.end(), "CellsFollower") != ordered_stages.end();
  const bool has_cells =
      std::find(ordered_stages.begin(), ordered_stages.end(), "Cells") != ordered_stages.end();
  if (has_cells_follower && has_cells) {
    tiles.push_back(StageStripTile{.label = "CellsAll", .pipelines = {"CellsFollower", "Cells"}});
  }

  if (!current_graphics.empty()) {
    tiles.push_back(StageStripTile{.label = "Full", .pipelines = current_graphics});
  }

  if (tiles.empty()) {
    tiles = {
        StageStripTile{.label = "Landscape", .pipelines = {"Landscape"}},
    };
  }

  return tiles;
}

const std::unordered_map<std::string, std::vector<std::string>> &tile_aliases() {
  static const std::unordered_map<std::string, std::vector<std::string>> aliases = {
      {"debug", {"LandscapeDebug"}},
      {"stage1", {"LandscapeStage1"}},
      {"stage2", {"LandscapeStage2"}},
      {"normals", {"LandscapeNormals"}},
      {"landscape", {"Landscape"}},
      {"sky", {"Sky"}},
      {"terrainbox", {"TerrainBox"}},
      {"cells", {"Cells"}},
      {"cellsfollower", {"CellsFollower"}},
      {"cellsall", {"CellsFollower", "Cells"}},
      {"cellsonly", {"CellsFollower", "Cells"}},
      {"full", {"Sky", "Landscape", "TerrainBox", "CellsFollower", "Cells"}},
  };
  return aliases;
}

std::vector<std::string> resolve_pipeline_alias(const std::string &token) {
  std::string normalized = token;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  const auto alias_it = tile_aliases().find(normalized);
  if (alias_it != tile_aliases().end()) {
    return alias_it->second;
  }

  return {token};
}

std::string auto_label_from_sources(const std::vector<std::string> &sources) {
  if (sources.empty()) {
    return "Tile";
  }
  if (sources.size() == 1) {
    return sources.front();
  }

  std::string label{};
  for (size_t i = 0; i < sources.size(); ++i) {
    if (i > 0) {
      label += "+";
    }
    label += sources[i];
  }
  return label;
}

void initialize_stage_strip_cache() {
  const char *enabled_raw = std::getenv("CE_RENDER_STAGE_STRIP");
  g_stage_strip_cache.enabled = enabled_raw ? CE::Runtime::env_truthy(enabled_raw) : true;

  if (const char *height_raw = std::getenv("CE_RENDER_STAGE_STRIP_HEIGHT")) {
    char *end = nullptr;
    const long parsed = std::strtol(height_raw, &end, 10);
    if (end != height_raw && *end == '\0' && parsed > 0) {
      g_stage_strip_cache.custom_height = static_cast<int32_t>(parsed);
    }
  }

  if (const char *padding_raw = std::getenv("CE_RENDER_STAGE_STRIP_PADDING")) {
    char *end = nullptr;
    const long parsed = std::strtol(padding_raw, &end, 10);
    if (end != padding_raw && *end == '\0' && parsed >= 0) {
      g_stage_strip_cache.custom_padding = static_cast<int32_t>(parsed);
    }
  }

  const char *raw_tiles = std::getenv("CE_RENDER_STAGE_TILES");
  if (!raw_tiles || std::string(raw_tiles).empty()) {
    g_stage_strip_cache.tiles = default_tiles();
  } else {
    const std::vector<std::string> tile_specs = split_trimmed(raw_tiles, ',');
    std::vector<StageStripTile> parsed_tiles{};
    parsed_tiles.reserve(tile_specs.size());

    for (const std::string &tile_spec : tile_specs) {
      const std::vector<std::string> source_specs = split_trimmed(tile_spec, '+');
      if (source_specs.empty()) {
        continue;
      }

      StageStripTile tile{};
      for (const std::string &source : source_specs) {
        const std::vector<std::string> resolved = resolve_pipeline_alias(source);
        tile.pipelines.insert(tile.pipelines.end(), resolved.begin(), resolved.end());
      }
      tile.label = auto_label_from_sources(source_specs);
      parsed_tiles.push_back(tile);
    }

    if (parsed_tiles.empty()) {
      g_stage_strip_cache.tiles = default_tiles();
    } else {
      g_stage_strip_cache.tiles = std::move(parsed_tiles);
    }
  }

  const char *raw_labels = std::getenv("CE_RENDER_STAGE_TILE_LABELS");
  if (raw_labels && std::string(raw_labels).size() > 0) {
    const std::vector<std::string> labels = split_trimmed(raw_labels, ',');
    const size_t assign_count = std::min(labels.size(), g_stage_strip_cache.tiles.size());
    for (size_t i = 0; i < assign_count; ++i) {
      if (!labels[i].empty()) {
        g_stage_strip_cache.tiles[i].label = labels[i];
      }
    }
  }

  g_stage_strip_cache.initialized = true;
}

void ensure_stage_strip_cache() {
  std::call_once(g_stage_strip_once, initialize_stage_strip_cache);
}

} // namespace

StageStripConfig get_stage_strip_config(const VkExtent2D &extent) {
  ensure_stage_strip_cache();
  StageStripConfig config{};
  config.enabled = g_stage_strip_cache.enabled;

  const uint32_t max_reasonable_height = std::max<uint32_t>(extent.height / 2, 1);
  if (g_stage_strip_cache.custom_height > 0) {
    config.strip_height_px =
        std::clamp<uint32_t>(static_cast<uint32_t>(g_stage_strip_cache.custom_height),
                             32,
                             max_reasonable_height);
  } else {
    config.strip_height_px = std::clamp<uint32_t>(extent.height / 5, 80, max_reasonable_height);
  }

  if (g_stage_strip_cache.custom_padding >= 0) {
    config.padding_px =
        std::clamp<uint32_t>(static_cast<uint32_t>(g_stage_strip_cache.custom_padding), 0, 64);
  }

  return config;
}

const std::vector<StageStripTile> &get_stage_strip_tiles() {
  ensure_stage_strip_cache();
  return g_stage_strip_cache.tiles;
}

bool is_stage_strip_enabled() {
  ensure_stage_strip_cache();
  return g_stage_strip_cache.enabled;
}

void log_stage_strip_tiles() {
  static bool logged = false;
  if (logged) {
    return;
  }

  ensure_stage_strip_cache();
  logged = true;

  if (!g_stage_strip_cache.enabled) {
    return;
  }

  Log::text("{ [*] }", "Stage strip tiles (index: label -> pipelines)");
  for (size_t i = 0; i < g_stage_strip_cache.tiles.size(); ++i) {
    const StageStripTile &tile = g_stage_strip_cache.tiles[i];
    std::string pipeline_list{};
    for (size_t j = 0; j < tile.pipelines.size(); ++j) {
      if (j > 0) {
        pipeline_list += ", ";
      }
      pipeline_list += tile.pipelines[j];
    }
    Log::text("{ [*] }",
              std::to_string(i) + ": " + tile.label + " -> " + pipeline_list);
  }
}

const StageStripTile *get_stage_strip_tile(size_t index) {
  ensure_stage_strip_cache();
  if (index >= g_stage_strip_cache.tiles.size()) {
    return nullptr;
  }
  return &g_stage_strip_cache.tiles[index];
}

int32_t find_stage_strip_tile_index(const VkExtent2D &extent,
                                    float normalized_x,
                                    float normalized_y) {
  ensure_stage_strip_cache();
  if (!g_stage_strip_cache.enabled) {
    return -1;
  }

  const StageStripConfig stage_strip = get_stage_strip_config(extent);
  const bool strip_active =
      stage_strip.enabled && extent.height > (stage_strip.strip_height_px + 1);
  if (!strip_active) {
    return -1;
  }

  const std::vector<StageStripTile> &tiles = g_stage_strip_cache.tiles;
  if (tiles.empty()) {
    return -1;
  }

  normalized_x = std::clamp(normalized_x, 0.0f, 1.0f);
  normalized_y = std::clamp(normalized_y, 0.0f, 1.0f);

  const uint32_t pixel_x =
      static_cast<uint32_t>(normalized_x * static_cast<float>(extent.width));
  const uint32_t pixel_y =
      static_cast<uint32_t>(normalized_y * static_cast<float>(extent.height));

  const uint32_t tile_count = static_cast<uint32_t>(tiles.size());
  const uint32_t padding = stage_strip.padding_px;
  const uint32_t strip_height = stage_strip.strip_height_px;
  const uint32_t reserved_padding = padding * 2;
  const uint32_t usable_width =
      (extent.width > reserved_padding) ? (extent.width - reserved_padding) : extent.width;

  // Calculate max tiles per row based on available width (minimum 60px per tile)
  const uint32_t tile_width_min = 60;
  const uint32_t max_tiles_per_row =
      std::max<uint32_t>(usable_width / (tile_width_min + padding), 1);
  const uint32_t tiles_per_row = std::min(max_tiles_per_row, tile_count);
  const uint32_t tile_width = usable_width / tiles_per_row;
  const uint32_t tile_height =
      std::max<uint32_t>((strip_height > 2 * padding) ? (strip_height - 2 * padding)
                                                       : strip_height,
                         1);

  // Calculate which row the click is in
  const uint32_t row_height = tile_height + padding;
  const uint32_t clicked_row = pixel_y > padding ? (pixel_y - padding) / row_height : 0;
  const uint32_t row_start_tile = clicked_row * tiles_per_row;

  if (row_start_tile >= tile_count) {
    return -1;  // Click is beyond available tiles
  }

  const uint32_t row_end_tile = std::min(row_start_tile + tiles_per_row, tile_count);

  for (uint32_t tile = row_start_tile; tile < row_end_tile; ++tile) {
    const uint32_t tile_in_row = tile - row_start_tile;
    const uint32_t tile_x = padding + tile_in_row * (tile_width + padding);
    const uint32_t clamped_width =
        std::min<uint32_t>(tile_width,
                           extent.width - std::min(tile_x, extent.width));

    const uint32_t y0_tile = padding + clicked_row * row_height;
    const uint32_t y1_tile = y0_tile + tile_height;

    const uint32_t x0 = tile_x;
    const uint32_t x1 = x0 + clamped_width;

    if (pixel_x >= x0 && pixel_x < x1 && pixel_y >= y0_tile && pixel_y < y1_tile) {
      return static_cast<int32_t>(tile);
    }
  }

  return -1;
}

} // namespace CE::RenderGUI
