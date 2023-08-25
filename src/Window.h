#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>
#include <iostream>

class Window {
 public:
  Window(const Window&) = delete;
  static Window& get() { return mainWindow; }

  GLFWwindow* window;
  bool framebufferResized;

  struct DisplayConfiguration {
      const char* title{ "G E N E R A T I O N S" };
      uint16_t width = 3800;
      uint16_t height = 1080;
  } display;

  struct Mouse {
    float pressDelay = 0.18f;
    float speed = 0.5f;

    struct Button {
      glm::vec2 position;
    };
    std::array<Button, 3> buttonClick;
    std::array<Button, 3> buttonDown;
    std::array<Button, 3> previousButtonDown;
  } mouse;

  void setMouse();

 private:
  Window();
  ~Window();

  void initWindow();
  static void windowResize(GLFWwindow* win, int width, int height);

  static Window mainWindow;
};
