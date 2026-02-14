#include "Window.h"

#include "core/Log.h"

#include <array>
#include <unordered_map>
#include <vector>

Window::Window() : framebuffer_resized{false}, window{nullptr} {
  Log::log_title();
  Log::text("{ [-] }", "constructing Window");
  init_window();
}

Window::~Window() {
  Log::text("{ [-] }", "destructing Window");
  Log::log_footer();

  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::init_window() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window =
      glfwCreateWindow(display.width, display.height, display.title, nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, window_resize);
  Log::text("{ [*] }", "Window initialized", display.width, "*", display.height);
}

void Window::window_resize(GLFWwindow *win, int width, int height) {
  auto app = reinterpret_cast<Window *>(glfwGetWindowUserPointer(win));
  app->framebuffer_resized = true;
  app->display.width = width;
  app->display.height = height;
  Log::text("{ [*] }", "Window resized to", width, "*", height);
}

void Window::poll_input() {
  glfwPollEvents();
  set_mouse();

  escape_pressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

  const bool f12Down = glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS;
  screenshot_pressed = f12Down && !screenshot_key_down;
  screenshot_key_down = f12Down;
}

bool Window::consume_screenshot_pressed() {
  if (!screenshot_pressed) {
    return false;
  }
  screenshot_pressed = false;
  return true;
}

void Window::set_mouse() {
  int newState = GLFW_RELEASE;
  static int buttonType = -1;
  constexpr std::array<int, 3> mouseButtonTypes{
      GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE};

  for (const int mouseButtonType : mouseButtonTypes) {
    if (glfwGetMouseButton(window, mouseButtonType) == GLFW_PRESS) {
      newState = GLFW_PRESS;
      buttonType = mouseButtonType;
      break;
    }
  }

  if (buttonType != -1) {
    static int oldState = GLFW_RELEASE;
    double xpos, ypos;
    static float pressTime = 0.0f;

    glfwGetCursorPos(window, &xpos, &ypos);
    const float x = static_cast<float>(xpos) / display.width;
    const float y = static_cast<float>(ypos) / display.height;

    static const std::unordered_map<int, std::string> buttonMappings = {
        {GLFW_MOUSE_BUTTON_LEFT, "{ --> } Left Mouse Button"},
        {GLFW_MOUSE_BUTTON_RIGHT, "{ --> } Right Mouse Button"},
        {GLFW_MOUSE_BUTTON_MIDDLE, "{ --> } Middle Mouse Button"}};

    switch (oldState) {
      case GLFW_PRESS: {
        if (newState == GLFW_RELEASE) {
          const std::unordered_map<int, std::string>::const_iterator &buttonMapping =
              buttonMappings.find(buttonType);
          if (buttonMapping != buttonMappings.end()) {
            const std::string &message = buttonMapping->second;
            mouse.button_click[buttonType].position = glm::vec2{x, y};

            Log::text(message + " clicked at",
                      mouse.button_click[buttonType].position.x,
                      ":",
                      mouse.button_click[buttonType].position.y);
          }
        } else {
          const float currentTime = static_cast<float>(glfwGetTime());
          const float timer = currentTime - pressTime;
          if (timer >= mouse.press_delay) {
            const std::unordered_map<int, std::string>::const_iterator &buttonMapping =
                buttonMappings.find(buttonType);
            if (buttonMapping != buttonMappings.end()) {
              const std::string &message = buttonMapping->second;
                const glm::vec2 normalizedCoords =
                  glm::vec2(x, y) * glm::vec2(2.0f, 2.0f) - glm::vec2(1.0f, 1.0f);
              mouse.button_down[buttonType].position += normalizedCoords * mouse.speed;

              // Log::text(message + " moved to",
              //              mouse.button_down[buttonType].position.x, ":",
              //              mouse.button_down[buttonType].position.y);
            }
          }
        }
        break;
      }
      case GLFW_RELEASE: {
        pressTime = (newState == GLFW_PRESS) ? static_cast<float>(glfwGetTime()) : 0.0f;
        break;
      }
    }
    oldState = newState;
  }
}
