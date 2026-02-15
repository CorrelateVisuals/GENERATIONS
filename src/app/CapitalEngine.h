#pragma once
#include "render/Mechanics.h"
#include "render/FrameContext.h"
#include "render/Pipelines.h"
#include "render/Resources.h"

#include <memory>
#include <string>

class CapitalEngine {
public:
  CapitalEngine();
  CapitalEngine(const CapitalEngine &) = delete;
  CapitalEngine &operator=(const CapitalEngine &) = delete;
  CapitalEngine(CapitalEngine &&) = delete;
  CapitalEngine &operator=(CapitalEngine &&) = delete;
  ~CapitalEngine();

  void main_loop();

private:
  VulkanMechanics mechanics;
  std::unique_ptr<Resources> resources;
  std::unique_ptr<Pipelines> pipelines;
  std::unique_ptr<FrameContext> frame_context;

  uint32_t last_presented_image_index{0};
  uint32_t last_submitted_frame_index{0};

  void recreate_swapchain();
  void draw_frame();
  void take_screenshot(const std::string &tag = "");
  
  struct ScreenshotState {
    bool first_loop_screenshot_captured;
    bool startup_screenshot_framed;
    std::vector<uint32_t> startup_screenshot_presets;
    size_t startup_screenshot_preset_index;
    bool startup_screenshot_pending_capture;
    std::chrono::steady_clock::time_point startup_screenshot_ready_at;
    std::chrono::steady_clock::time_point startup_screenshot_capture_at;
  };
  
  ScreenshotState initialize_screenshot_state() const;
  void handle_startup_screenshot_cycle(ScreenshotState &state);
  void handle_startup_screenshot_single(ScreenshotState &state);
  bool should_capture_screenshot(const ScreenshotState &state) const;
  void capture_preset_screenshot(ScreenshotState &state);
  void process_frame_update(Window &window);
  void process_screenshot_handling(ScreenshotState &state, Window &window);
  std::filesystem::path get_screenshot_directory() const;
  std::filesystem::path find_project_root() const;
  std::tm get_current_time_info() const;
  std::string build_screenshot_filename(const std::string &tag) const;
  std::string format_screenshot_name(const std::tm &timeInfo, int milliseconds, const std::string &tag) const;
};
