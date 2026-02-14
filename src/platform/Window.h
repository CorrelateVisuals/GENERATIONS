#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>

class Window {
public:
  Window(const Window &) = delete;

  struct DisplayConfiguration {
    DisplayConfiguration(const char *t, uint16_t w, uint16_t h)
        : title{t}, width{w}, height{h} {}

    const char *title;
    uint16_t width;
    uint16_t height;
  };

  struct Mouse {
    Mouse(float d, float s) : press_delay(d), speed(s) {}
    float press_delay;
    float speed;

    struct Button {
      glm::vec2 position;
    };
    std::array<Button, 3> button_click{};
    std::array<Button, 3> button_down{};
    std::array<Button, 3> previous_button_down{};
  };

  bool framebuffer_resized;
  GLFWwindow *window;
  DisplayConfiguration display{
#ifdef __linux__
      "GENERATIONS", 1920, 1080
#else
      "GENERATIONS", 3840, 1080
#endif
  };
  Mouse mouse{0.18f, 0.5f};

  static Window &get() {
    return main_window;
  }
  void poll_input();
  bool is_escape_pressed() const {
    return escape_pressed;
  }
  bool consume_screenshot_pressed();

private:
  void set_mouse();
  bool escape_pressed{false};
  bool screenshot_key_down{false};
  bool screenshot_pressed{false};

  Window();
  ~Window();

  void init_window();
  static void window_resize(GLFWwindow *win, int width, int height);

  static Window main_window;
};
