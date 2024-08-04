#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>

class Window {
 public:
  Window(const Window&) = delete;

  bool framebufferResized;

  struct DisplayConfiguration {
    DisplayConfiguration(const char* t, uint16_t w, uint16_t h)
        : title{t}, width{w}, height{h} {}

    const char* title;
    uint16_t width;
    uint16_t height;
  };

  struct Mouse {
    Mouse(float d, float s) : pressDelay(d), speed(s) {}
    float pressDelay;
    float speed;

    struct Button {
      glm::vec2 position;
    };
    std::array<Button, 3> buttonClick;
    std::array<Button, 3> buttonDown;
    std::array<Button, 3> previousButtonDown;
  };

  GLFWwindow* window;
  DisplayConfiguration display{"GENERATIONS", 3840, 1080};
  Mouse mouse{0.18f, 0.5f};

  static Window& get() { return mainWindow; }
  void setMouse();

 private:
  Window();
  ~Window();

  void initWindow();
  static void windowResize(GLFWwindow* win, int width, int height);

  static Window mainWindow;
};
