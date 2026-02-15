#include "CapitalEngine.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "io/Screenshot.h"
#include "platform/Window.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

CapitalEngine::CapitalEngine() {
  const CE::Runtime::TerrainSettings &terrain_settings = CE::Runtime::get_terrain_settings();
  resources = std::make_unique<Resources>(mechanics, terrain_settings);
  pipelines = std::make_unique<Pipelines>(mechanics, *resources);
  frame_context = std::make_unique<FrameContext>(mechanics, *resources, *pipelines);
  
  Log::text(Log::Style::header_guard);
  Log::text("| CAPITAL Engine");
}

CapitalEngine::~CapitalEngine() {
  Log::text(Log::Style::header_guard);
  Log::text("| CAPITAL Engine");
  Log::text(Log::Style::header_guard);
}

void CapitalEngine::recreate_swapchain() {
  mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                               mechanics.queues,
                               mechanics.sync_objects,
                               *pipelines,
                               *resources);
}

CapitalEngine::ScreenshotState CapitalEngine::initialize_screenshot_state() const {
  const bool startup_screenshot_enabled = CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT");
  const bool cycle_enabled = CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT_CYCLE");
  ScreenshotState state{
      .first_loop_screenshot_captured = !startup_screenshot_enabled,
      .startup_screenshot_framed = !startup_screenshot_enabled,
      .startup_screenshot_presets = (startup_screenshot_enabled && cycle_enabled) ? std::vector<uint32_t>{1, 2, 3, 4} : std::vector<uint32_t>{},
      .startup_screenshot_preset_index = 0,
      .startup_screenshot_pending_capture = false,
      .startup_screenshot_ready_at = std::chrono::steady_clock::now() + std::chrono::seconds(1),
      .startup_screenshot_capture_at = std::chrono::steady_clock::now() + std::chrono::seconds(1)};
  return state;
}

bool CapitalEngine::should_capture_screenshot(const ScreenshotState &state) const {
  return !state.first_loop_screenshot_captured &&
         std::chrono::steady_clock::now() >= state.startup_screenshot_ready_at;
}

void CapitalEngine::capture_preset_screenshot(ScreenshotState &state) {
  const uint32_t current_preset = state.startup_screenshot_presets[state.startup_screenshot_preset_index];
  Log::text("{ >>> }", "Startup screenshot capture for preset " + std::to_string(current_preset));
  take_screenshot("preset" + std::to_string(current_preset));
  ++state.startup_screenshot_preset_index;
  state.startup_screenshot_pending_capture = false;
  if (state.startup_screenshot_preset_index >= state.startup_screenshot_presets.size()) {
    state.first_loop_screenshot_captured = true;
  }
}

void CapitalEngine::handle_startup_screenshot_cycle(ScreenshotState &state) {
  if (!state.startup_screenshot_pending_capture) {
    const uint32_t current_preset = state.startup_screenshot_presets[state.startup_screenshot_preset_index];
    resources->world._camera.set_preset_view(current_preset);
    state.startup_screenshot_pending_capture = true;
    state.startup_screenshot_capture_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
  } else if (std::chrono::steady_clock::now() >= state.startup_screenshot_capture_at) {
    capture_preset_screenshot(state);
  }
}

void CapitalEngine::handle_startup_screenshot_single(ScreenshotState &state) {
  if (!state.startup_screenshot_framed) {
    state.startup_screenshot_framed = true;
    resources->world._camera.set_preset_view(4);
  }
  state.first_loop_screenshot_captured = true;
  Log::text("{ >>> }", "Main loop startup screenshot capture");
  take_screenshot();
}

void CapitalEngine::process_frame_update(Window &window) {
  window.poll_input();
  resources->world._time.run();
  mechanics.main_device.maybe_log_gpu_runtime_sample();
  draw_frame();
}

void CapitalEngine::process_screenshot_handling(ScreenshotState &state, Window &window) {
  if (should_capture_screenshot(state)) {
    const bool cycle_enabled = CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT_CYCLE");
    if (cycle_enabled && !state.startup_screenshot_presets.empty()) {
      handle_startup_screenshot_cycle(state);
    } else {
      handle_startup_screenshot_single(state);
    }
  }
  if (window.consume_screenshot_pressed()) {
    Log::text("{ >>> }", "F12 pressed - capturing screenshot");
    take_screenshot();
  }
}

void CapitalEngine::main_loop() {
  Log::text(Log::Style::header_guard);
  Log::text("{ Main Loop }");
  Log::measure_elapsed_time();
  ScreenshotState screenshot_state = initialize_screenshot_state();
  Window &main_window = Window::get();
  while (!glfwWindowShouldClose(main_window.window)) {
    process_frame_update(main_window);
    process_screenshot_handling(screenshot_state, main_window);
    if (main_window.is_escape_pressed()) {
      break;
    }
  }
  vkDeviceWaitIdle(mechanics.main_device.logical_device);
  Log::measure_elapsed_time();
  Log::text(Log::Style::header_guard);
}

void CapitalEngine::draw_frame() {
  frame_context->draw_frame(last_presented_image_index,
                            last_submitted_frame_index,
                            [this]() { recreate_swapchain(); });
}

std::filesystem::path CapitalEngine::find_project_root() const {
  std::filesystem::path output_root = std::filesystem::current_path();
  if (!std::filesystem::exists(output_root / "CMakeLists.txt") &&
      std::filesystem::exists(output_root.parent_path() / "CMakeLists.txt")) {
    output_root = output_root.parent_path();
  }
  return output_root;
}

std::filesystem::path CapitalEngine::get_screenshot_directory() const {
  const std::filesystem::path screenshot_dir = find_project_root() / "screenshot";
  std::filesystem::create_directories(screenshot_dir);
  return screenshot_dir;
}

std::tm CapitalEngine::get_current_time_info() const {
  const std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm timeInfo{};
#ifdef __linux__
  localtime_r(&timestamp, &timeInfo);
#else
  localtime_s(&timeInfo, &timestamp);
#endif
  return timeInfo;
}

std::string CapitalEngine::format_screenshot_name(const std::tm &timeInfo, int milliseconds, const std::string &tag) const {
  std::ostringstream nameBuilder;
  nameBuilder << "screenshot_" << std::put_time(&timeInfo, "%Y%m%d_%H%M%S") << "_" << std::setw(3) << std::setfill('0') << milliseconds;
  if (!tag.empty()) {
    nameBuilder << "_" << tag;
  }
  nameBuilder << ".png";
  return nameBuilder.str();
}

std::string CapitalEngine::build_screenshot_filename(const std::string &tag) const {
  const std::filesystem::path screenshot_dir = get_screenshot_directory();
  const std::tm timeInfo = get_current_time_info();
  const auto now = std::chrono::system_clock::now();
  const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  const std::string name = format_screenshot_name(timeInfo, milliseconds.count(), tag);
  return (screenshot_dir / name).string();
}

void CapitalEngine::take_screenshot(const std::string &tag) {
  vkWaitForFences(mechanics.main_device.logical_device,
                  1,
                  &mechanics.sync_objects.graphics_in_flight_fences[last_submitted_frame_index],
                  VK_TRUE,
                  UINT64_MAX);
  const std::string filename = build_screenshot_filename(tag);
  CE::Screenshot::capture(mechanics.swapchain.images[last_presented_image_index].image,
                          mechanics.swapchain.extent,
                          mechanics.swapchain.image_format,
                          resources->commands.pool,
                          mechanics.queues.graphics_queue,
                          filename);
}
