#include "Window.h"
#include "CapitalEngine.h"

#include <unordered_map>

Window Window::mainWindow;

Window::Window() : window{nullptr}, framebufferResized{false} {
  Log::logTitle();
  Log::text("{ [-] }", "constructing Window");
  initWindow();
}

Window::~Window() {
  Log::text("{ [-] }", "destructing Window");
  Log::logFooter();

  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(display.width, display.height, display.title,
                            nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, windowResize);
  Log::text("{ [*] }", "Window initialized", display.width, "*",
            display.height);
}

void Window::windowResize(GLFWwindow* win, int width, int height) {
  auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
  app->framebufferResized = true;
  app->display.width = width;
  app->display.height = height;
  Log::text("{ [*] }", "Window resized to", width, "*", height);
}

void Window::setMouse() {
  int newState = GLFW_RELEASE;
  static int buttonType = -1;
  const static std::vector<uint32_t> mouseButtonTypes{GLFW_MOUSE_BUTTON_LEFT,
                                                      GLFW_MOUSE_BUTTON_RIGHT,
                                                      GLFW_MOUSE_BUTTON_MIDDLE};

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
          const std::unordered_map<int, std::string>::const_iterator&
              buttonMapping = buttonMappings.find(buttonType);
          if (buttonMapping != buttonMappings.end()) {
            const std::string& message = buttonMapping->second;
            mouse.buttonClick[buttonType].position = glm::vec2{x, y};

            Log::text(message + " clicked at",
                      mouse.buttonClick[buttonType].position.x, ":",
                      mouse.buttonClick[buttonType].position.y);
          }
        } else {
          const float currentTime = static_cast<float>(glfwGetTime());
          const float timer = currentTime - pressTime;
          if (timer >= mouse.pressDelay) {
            const std::unordered_map<int, std::string>::const_iterator&
                buttonMapping = buttonMappings.find(buttonType);
            if (buttonMapping != buttonMappings.end()) {
              const std::string& message = buttonMapping->second;
              glm::vec2 normalizedCoords =
                  glm::vec2(x, y) * glm::vec2(2.0f, 2.0f) -
                  glm::vec2(1.0f, 1.0f);
              mouse.buttonDown[buttonType].position +=
                  normalizedCoords * mouse.speed;

              // Log::text(message + " moved to",
              //              mouse.buttonDown[buttonType].position.x, ":",
              //              mouse.buttonDown[buttonType].position.y);
            }
          }
        }
        break;
      }
      case GLFW_RELEASE: {
        pressTime =
            (newState == GLFW_PRESS) ? static_cast<float>(glfwGetTime()) : 0.0f;
        break;
      }
    }
    oldState = newState;
  }
}
