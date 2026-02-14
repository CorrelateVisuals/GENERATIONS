#include "CapitalEngine.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "io/Screenshot.h"
#include "platform/Window.h"
#include "base/VulkanUtils.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

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

void CapitalEngine::main_loop() {
  Log::text(Log::Style::header_guard);
  Log::text("{ Main Loop }");
  Log::measure_elapsed_time();

  const bool startup_screenshot_enabled =
      CE::Runtime::env_flag_enabled("CE_STARTUP_SCREENSHOT");
  bool first_loop_screenshot_captured = !startup_screenshot_enabled;
  const auto startup_screenshot_ready_at =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  Window &main_window = Window::get();

  while (!glfwWindowShouldClose(main_window.window)) {
    main_window.poll_input();
    resources->world._time.run();

    draw_frame();

    if (!first_loop_screenshot_captured &&
        std::chrono::steady_clock::now() >= startup_screenshot_ready_at) {
      first_loop_screenshot_captured = true;
      Log::text("{ >>> }", "Main loop startup screenshot capture");
      take_screenshot();
    }

    if (main_window.consume_screenshot_pressed()) {
      Log::text("{ >>> }", "F12 pressed - capturing screenshot");
      take_screenshot();
    }

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

void CapitalEngine::take_screenshot() {
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
  nameBuilder << "screenshot_" << std::put_time(&timeInfo, "%Y%m%d_%H%M%S") << ".png";
  const std::string filename = (screenshot_dir / nameBuilder.str()).string();

  CE::Screenshot::capture(mechanics.swapchain.images[last_presented_image_index].image,
                          mechanics.swapchain.extent,
                          mechanics.swapchain.image_format,
                          resources->commands.pool,
                          mechanics.queues.graphics_queue,
                          filename);
}
