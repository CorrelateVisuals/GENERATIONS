#include "CapitalEngine.h"
#include "engine/Log.h"
#include "world/RuntimeConfig.h"
#include "library/Screenshot.h"
#include "control/Window.h"
#include "control/gui.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

CapitalEngine::CapitalEngine() {
  const CE::Runtime::TerrainSettings &terrain_settings = CE::Runtime::get_terrain_settings();
  resources = std::make_unique<VulkanResources>(mechanics, terrain_settings);
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

void CapitalEngine::main_loop() {
  Log::text(Log::Style::header_guard);
  Log::text("{ Main Loop }");
  Log::measure_elapsed_time();

  CE::RenderGUI::log_stage_strip_tiles();
  if (CE::RenderGUI::is_stage_strip_enabled()) {
    const bool strip_full = CE::Runtime::env_flag_enabled("CE_RENDER_STAGE_STRIP_FULL");
    Log::text("{ STRIP }",
              "Live strip mode",
              strip_full ? "FULL" : "LIGHTWEIGHT");
  }

  const bool camera_tuning_enabled =
      CE::Runtime::env_flag_enabled("CE_CAMERA_TUNING");
  if (camera_tuning_enabled) {
    Log::text("{ Cam }",
              "Camera tuning enabled (T toggle, ,/. select, [] adjust)");
  }

  const bool startup_screenshot_enabled =
      CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT");
  const bool startup_screenshot_cycle_enabled =
      CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT_CYCLE");

  bool first_loop_screenshot_captured = !startup_screenshot_enabled;
  bool startup_screenshot_framed = !startup_screenshot_enabled;
  std::vector<uint32_t> startup_screenshot_presets;
  if (startup_screenshot_enabled && startup_screenshot_cycle_enabled) {
    startup_screenshot_presets = {1, 2, 3, 4};
  }
  size_t startup_screenshot_preset_index = 0;
  bool startup_screenshot_pending_capture = false;
  auto startup_screenshot_ready_at =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  auto startup_screenshot_capture_at = startup_screenshot_ready_at;
  Window &main_window = Window::get();

  const std::string base_window_title =
      main_window.display.title ? std::string(main_window.display.title) : std::string("GENERATIONS");
  constexpr auto fps_title_update_interval = std::chrono::milliseconds(250);
  auto fps_window_start = std::chrono::steady_clock::now();
  uint32_t fps_frame_count = 0;

  while (!glfwWindowShouldClose(main_window.window)) {
    main_window.poll_input();

    glm::vec2 left_click_position{0.0f, 0.0f};
    if (main_window.consume_left_click(left_click_position)) {
      const int32_t clicked_tile_index =
          CE::RenderGUI::find_stage_strip_tile_index(mechanics.swapchain.extent,
                                                     left_click_position.x,
                                                     left_click_position.y);
      if (clicked_tile_index >= 0) {
        const CE::RenderGUI::StageStripTile *tile =
            CE::RenderGUI::get_stage_strip_tile(static_cast<size_t>(clicked_tile_index));
        if (tile) {
          // Check if this is a preset tile
          if (tile->preset_index >= 0) {
            resources->world._camera.set_preset_view(
                static_cast<uint32_t>(tile->preset_index));
            Log::text("{ >>> }", "Stage strip preset: " + tile->label);
          } else if (!tile->pipelines.empty()) {
            // This is a render pipeline tile
            CE::Runtime::RenderGraph updated_graph{};
            if (const CE::Runtime::RenderGraph *existing_graph =
                    CE::Runtime::get_render_graph()) {
              updated_graph = *existing_graph;
            }

            std::vector<CE::Runtime::RenderNode> retained_nodes{};
            retained_nodes.reserve(updated_graph.nodes.size());
            for (const CE::Runtime::RenderNode &node : updated_graph.nodes) {
              if (node.stage != CE::Runtime::RenderStage::Graphics) {
                retained_nodes.push_back(node);
              }
            }

            for (const std::string &pipeline_name : tile->pipelines) {
              retained_nodes.push_back(CE::Runtime::RenderNode{
                  .stage = CE::Runtime::RenderStage::Graphics,
                  .pipeline = pipeline_name,
                  .draw_op = CE::Runtime::get_graphics_draw_op_id(pipeline_name),
              });
            }

            updated_graph.nodes = std::move(retained_nodes);
            CE::Runtime::set_render_graph(updated_graph);
            Log::text("{ >>> }", "Stage strip selected: " + tile->label);
          }
        }
      }
    }

    resources->world._time.run();
    mechanics.main_device.maybe_log_gpu_runtime_sample();

    draw_frame();
    ++fps_frame_count;

    const auto now = std::chrono::steady_clock::now();
    const auto fps_elapsed = now - fps_window_start;
    if (fps_elapsed >= fps_title_update_interval) {
      const double elapsed_seconds =
          std::chrono::duration_cast<std::chrono::duration<double>>(fps_elapsed).count();
      if (elapsed_seconds > 0.0) {
        const double fps = static_cast<double>(fps_frame_count) / elapsed_seconds;
        const double frame_ms = (fps > 0.0) ? (1000.0 / fps) : 0.0;

        std::ostringstream title_stream;
        title_stream << base_window_title << " | FPS " << std::fixed << std::setprecision(1)
                     << fps << " | " << std::setprecision(2) << frame_ms << " ms";
        glfwSetWindowTitle(main_window.window, title_stream.str().c_str());
      }

      fps_window_start = now;
      fps_frame_count = 0;
    }

    if (!first_loop_screenshot_captured &&
        std::chrono::steady_clock::now() >= startup_screenshot_ready_at) {
      if (startup_screenshot_cycle_enabled && !startup_screenshot_presets.empty()) {
        if (!startup_screenshot_pending_capture) {
          const uint32_t current_preset =
              startup_screenshot_presets[startup_screenshot_preset_index];
          resources->world._camera.set_preset_view(current_preset);
          startup_screenshot_pending_capture = true;
          startup_screenshot_capture_at =
              std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
        } else if (std::chrono::steady_clock::now() >= startup_screenshot_capture_at) {
          const uint32_t current_preset =
              startup_screenshot_presets[startup_screenshot_preset_index];
          Log::text("{ >>> }",
                    "Startup screenshot capture for preset " + std::to_string(current_preset));
          take_screenshot("preset" + std::to_string(current_preset));

          startup_screenshot_pending_capture = false;
          ++startup_screenshot_preset_index;
          if (startup_screenshot_preset_index >= startup_screenshot_presets.size()) {
            first_loop_screenshot_captured = true;
          }
        }
      } else {
        if (!startup_screenshot_framed) {
          startup_screenshot_framed = true;
          resources->world._camera.set_preset_view(4);
        }

        first_loop_screenshot_captured = true;
        Log::text("{ >>> }", "Main loop startup screenshot capture");
        take_screenshot();
      }
    }

    if (main_window.consume_screenshot_pressed()) {
      Log::text("{ >>> }", "F12 pressed - capturing screenshot");
      take_screenshot();
    }

    if (main_window.is_escape_pressed()) {
      break;
    }
  }
  glfwSetWindowTitle(main_window.window, base_window_title.c_str());
  vkDeviceWaitIdle(mechanics.main_device.logical_device);

  Log::measure_elapsed_time();
  Log::text(Log::Style::header_guard);
}

void CapitalEngine::draw_frame() {
  frame_context->draw_frame(last_presented_image_index,
                            last_submitted_frame_index,
                            [this]() { recreate_swapchain(); });
}

void CapitalEngine::take_screenshot(const std::string &tag) {
  vkWaitForFences(mechanics.main_device.logical_device,
                  1,
                  &mechanics.sync_objects.graphics_in_flight_fences[last_submitted_frame_index],
                  VK_TRUE,
                  UINT64_MAX);

  std::filesystem::path output_root = std::filesystem::current_path();
  if (!std::filesystem::exists(output_root / "CMakeLists.txt") &&
      std::filesystem::exists(output_root.parent_path() / "CMakeLists.txt")) {
    output_root = output_root.parent_path();
  }

  const std::filesystem::path screenshot_dir = output_root / "screenshot";
  std::filesystem::create_directories(screenshot_dir);

  const std::time_t timestamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm timeInfo{};
#ifdef __linux__
  localtime_r(&timestamp, &timeInfo);
#else
  localtime_s(&timeInfo, &timestamp);
#endif

  std::ostringstream nameBuilder;
  const auto now = std::chrono::system_clock::now();
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  nameBuilder << "screenshot_" << std::put_time(&timeInfo, "%Y%m%d_%H%M%S") << "_"
              << std::setw(3) << std::setfill('0') << milliseconds.count();
  if (!tag.empty()) {
    nameBuilder << "_" << tag;
  }
  nameBuilder << ".png";
  const std::string filename = (screenshot_dir / nameBuilder.str()).string();

  CE::Screenshot::capture(mechanics.swapchain.images[last_presented_image_index].image,
                          mechanics.swapchain.extent,
                          mechanics.swapchain.image_format,
                          resources->commands.pool,
                          mechanics.queues.graphics_queue,
                          filename);
}
