#include "gui.h"

#include "world/RuntimeConfig.h"
#include "engine/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iterator>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <imgui.h>

namespace CE::RenderGUI {

namespace {

constexpr int32_t kDisabledValue = -1;
constexpr int32_t kNonPresetTile = -1;
constexpr uint32_t kMaxStripRows = 2;
constexpr uint32_t kDefaultStripRows = 2;
constexpr uint32_t kMinCustomHeight = 32;
constexpr uint32_t kMinAutoHeight = 48;
constexpr uint32_t kPaddingMax = 64;

constexpr const char *kKeyPresetPrefix = "preset:";
constexpr const char *kKeyPipelinesPrefix = "pipelines:";
constexpr const char *kLabelTileFallback = "Tile";
constexpr const char *kLabelCurrent = "Current";
constexpr const char *kLabelCellsAll = "CellsAll";
constexpr const char *kLabelLandscape = "Landscape";

const std::vector<std::string> kStaticPreviewPipelines = {
    "Sky",
    "Landscape",
    "TerrainBox",
    "Cells",
    "CellsFollower",
};

const std::vector<std::string> kPreferredTileOrder = {
    "LandscapeDebug",
    "LandscapeStage1",
    "LandscapeStage2",
    "LandscapeNormals",
    "LandscapeStatic",
    "Sky",
    "Landscape",
    "TerrainBox",
    "Cells",
    "CellsFollower",
};

struct StageStripCache {
  bool initialized = false;
  bool enabled = true;
  int32_t custom_height = kDisabledValue;
  int32_t custom_padding = kDisabledValue;
  int32_t custom_max_rows = kDisabledValue;
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

std::vector<std::string> unique_preserve_order(const std::vector<std::string> &values) {
  std::vector<std::string> unique_values{};
  std::set<std::string> seen{};
  unique_values.reserve(values.size());
  for (const std::string &value : values) {
    if (value.empty()) {
      continue;
    }
    if (seen.insert(value).second) {
      unique_values.push_back(value);
    }
  }
  return unique_values;
}

std::string pipelines_key(const std::vector<std::string> &pipelines) {
  std::string key{};
  for (const std::string &pipeline : unique_preserve_order(pipelines)) {
    if (!key.empty()) {
      key.push_back('|');
    }
    key += pipeline;
  }
  return key;
}

void dedupe_tiles_in_place(std::vector<StageStripTile> &tiles) {
  std::set<std::string> seen_keys{};
  std::vector<StageStripTile> deduped{};
  deduped.reserve(tiles.size());

  for (StageStripTile &tile : tiles) {
    tile.pipelines = unique_preserve_order(tile.pipelines);

    std::string key{};
    if (tile.preset_index >= 0) {
      key = std::string(kKeyPresetPrefix) + std::to_string(tile.preset_index);
    } else {
      key = std::string(kKeyPipelinesPrefix) + pipelines_key(tile.pipelines);
    }

    if (!seen_keys.insert(key).second) {
      continue;
    }
    deduped.push_back(std::move(tile));
  }

  tiles = std::move(deduped);
}

void append_tile_if_unique(std::vector<StageStripTile> &tiles, StageStripTile tile) {
  tile.pipelines = unique_preserve_order(tile.pipelines);
  const std::string key = tile.preset_index >= 0
                              ? std::string(kKeyPresetPrefix) + std::to_string(tile.preset_index)
                              : std::string(kKeyPipelinesPrefix) + pipelines_key(tile.pipelines);

  const auto duplicate_it = std::find_if(tiles.begin(), tiles.end(), [&](const StageStripTile &existing) {
    const std::string existing_key = existing.preset_index >= 0
                                         ? std::string(kKeyPresetPrefix) + std::to_string(existing.preset_index)
                                         : std::string(kKeyPipelinesPrefix) + pipelines_key(existing.pipelines);
    return existing_key == key;
  });
  if (duplicate_it == tiles.end()) {
    tiles.push_back(std::move(tile));
  }
}

const std::vector<StageStripTile> &default_tiles() {
  static std::vector<StageStripTile> tiles{};
  if (!tiles.empty()) {
    return tiles;
  }

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

  std::set<std::string> used_names{};
  std::vector<std::string> ordered_stages{};
  ordered_stages.reserve(graphics_stage_names.size());

  for (const std::string &name : kPreferredTileOrder) {
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

  for (const std::string &stage : ordered_stages) {
    append_tile_if_unique(tiles, StageStripTile{.label = stage, .pipelines = {stage}});
  }

  const bool has_cells_follower =
      std::find(ordered_stages.begin(), ordered_stages.end(), "CellsFollower") != ordered_stages.end();
  const bool has_cells =
      std::find(ordered_stages.begin(), ordered_stages.end(), "Cells") != ordered_stages.end();
  if (has_cells_follower && has_cells) {
    append_tile_if_unique(tiles,
                          StageStripTile{.label = kLabelCellsAll,
                                         .pipelines = {"Cells", "CellsFollower"}});
  }

  if (!current_graphics.empty()) {
    append_tile_if_unique(tiles,
                          StageStripTile{.label = kLabelCurrent, .pipelines = current_graphics});
  }

  if (tiles.empty()) {
    tiles = {
        StageStripTile{.label = kLabelLandscape, .pipelines = {kLabelLandscape}},
    };
  }

  dedupe_tiles_in_place(tiles);

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
      {"cellsall", {"Cells", "CellsFollower"}},
      {"cellsonly", {"Cells", "CellsFollower"}},
      {"full", kStaticPreviewPipelines},
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
    return kLabelTileFallback;
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

  if (const char *rows_raw = std::getenv("CE_RENDER_STAGE_STRIP_MAX_ROWS")) {
    char *end = nullptr;
    const long parsed = std::strtol(rows_raw, &end, 10);
    if (end != rows_raw && *end == '\0' && parsed > 0) {
      g_stage_strip_cache.custom_max_rows = static_cast<int32_t>(parsed);
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

  dedupe_tiles_in_place(g_stage_strip_cache.tiles);

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
  if (g_stage_strip_cache.custom_max_rows > 0) {
    config.max_rows =
        std::clamp<uint32_t>(static_cast<uint32_t>(g_stage_strip_cache.custom_max_rows), 1, kMaxStripRows);
  } else {
    config.max_rows = kDefaultStripRows;
  }

  const uint32_t max_reasonable_height = std::max<uint32_t>(extent.height / 2, 1);
  if (g_stage_strip_cache.custom_height > 0) {
    config.strip_height_px =
        std::clamp<uint32_t>(static_cast<uint32_t>(g_stage_strip_cache.custom_height),
                             kMinCustomHeight,
                             max_reasonable_height);
  } else {
    config.strip_height_px = std::clamp<uint32_t>(extent.height / 15, kMinAutoHeight, max_reasonable_height);
  }

  config.strip_height_px =
      std::min<uint32_t>(config.strip_height_px * 2, max_reasonable_height);

    if (g_stage_strip_cache.custom_padding >= 0) {
    config.padding_px =
      std::clamp<uint32_t>(static_cast<uint32_t>(g_stage_strip_cache.custom_padding), 0, kPaddingMax);
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
  const uint32_t tile_count = static_cast<uint32_t>(g_stage_strip_cache.tiles.size());
  const uint32_t rows =
      std::max<uint32_t>(1, std::min<uint32_t>(stage_strip.max_rows, std::max<uint32_t>(tile_count, 1)));
  const uint32_t total_strip_height = rows * std::max<uint32_t>(stage_strip.strip_height_px, 1);
  const bool strip_active = stage_strip.enabled && extent.height > (total_strip_height + 1);
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

  const uint32_t tile_height = std::max<uint32_t>(stage_strip.strip_height_px, 1);
  const uint32_t columns = (tile_count + rows - 1) / rows;
  const uint32_t extent_width = std::max<uint32_t>(extent.width, 1);

  if (pixel_y >= rows * tile_height) {
    return -1;
  }

  for (uint32_t tile = 0; tile < tile_count; ++tile) {
    const uint32_t row = tile / columns;
    const uint32_t column = tile % columns;
    const uint32_t tile_x = (column * extent_width) / columns;
    const uint32_t tile_x_next = ((column + 1) * extent_width) / columns;
    const uint32_t tile_width = std::max<uint32_t>(tile_x_next - tile_x, 1);
    const uint32_t clamped_width =
        std::min<uint32_t>(tile_width, extent.width - std::min(tile_x, extent.width));
    const uint32_t tile_y = row * tile_height;
    const uint32_t tile_y_next = tile_y + tile_height;

    const uint32_t x0 = tile_x;
    const uint32_t x1 = x0 + clamped_width;

    if (pixel_x >= x0 && pixel_x < x1 && pixel_y >= tile_y && pixel_y < tile_y_next) {
      return static_cast<int32_t>(tile);
    }
  }

  return -1;
}

} // namespace CE::RenderGUI
