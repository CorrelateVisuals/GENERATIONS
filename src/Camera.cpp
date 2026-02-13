#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>

#include "Camera.h"
#include "Log.h"
#include "Window.h"

#include <algorithm>
#include <array>
#include <cmath>

void Camera::toggleMode() {
    if (mode == Mode::Panning) {
        syncArcballFromCurrentView();
        mode = Mode::Arcball;
        Log::text("{ Cam }", "Mode: Arcball");
        return;
    }

    mode = Mode::Panning;
    Log::text("{ Cam }", "Mode: Panning");
}

void Camera::syncArcballFromCurrentView() {
    const glm::vec3 forward = glm::normalize(front);
    const float targetDistance = std::clamp(arcballDistance, arcballMinDistance,
                                                                                    arcballMaxDistance);

    arcballTarget = position + forward * targetDistance;
    arcballDistance = glm::length(arcballTarget - position);
    if (arcballDistance <= 0.0001f) {
        arcballDistance = arcballMinDistance;
    }

    const glm::vec3 offset = position - arcballTarget;
    const float horizontal = std::sqrt(offset.x * offset.x + offset.y * offset.y);
    arcballYaw = std::atan2(offset.y, offset.x);
    arcballPitch = std::atan2(offset.z, std::max(horizontal, 0.0001f));
    arcballPitch = std::clamp(arcballPitch, glm::radians(-89.0f),
                                                        glm::radians(89.0f));
}

void Camera::applyPanningMode(const glm::vec2& leftButtonDelta,
                                                            const glm::vec2& rightButtonDelta) {
    glm::vec3 cameraRight = glm::normalize(glm::cross(front, up));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, front));

    position -= panningSpeed * leftButtonDelta.x * cameraRight;
    position -= panningSpeed * leftButtonDelta.y * cameraUp;

    position += zoomSpeed * rightButtonDelta.x * front;
    position.z = std::max(position.z, 0.0f);
}

void Camera::applyArcballMode(const glm::vec2& leftButtonDelta,
                                                            const glm::vec2& rightButtonDelta,
                                                            const glm::vec2& middleButtonDelta) {
    arcballYaw += leftButtonDelta.x * arcballRotateSpeed;
    arcballPitch -= leftButtonDelta.y * arcballRotateSpeed;
    arcballPitch = std::clamp(arcballPitch, glm::radians(-89.0f),
                                                        glm::radians(89.0f));

    const float zoomDelta = rightButtonDelta.x + middleButtonDelta.x;
    arcballDistance -= zoomDelta * arcballZoomSpeed * 10.0f;
    arcballDistance = std::clamp(arcballDistance, arcballMinDistance,
                                                             arcballMaxDistance);

    const glm::vec3 offsetDirection = glm::normalize(glm::vec3(
            std::cos(arcballPitch) * std::cos(arcballYaw),
            std::cos(arcballPitch) * std::sin(arcballYaw),
            std::sin(arcballPitch)));

    const glm::vec3 currentPosition = arcballTarget + offsetDirection * arcballDistance;
    const glm::vec3 currentFront = glm::normalize(arcballTarget - currentPosition);
    const glm::vec3 currentRight =
            glm::normalize(glm::cross(currentFront, glm::vec3(0.0f, -1.0f, 0.0f)));
    const glm::vec3 currentUp = glm::normalize(glm::cross(currentRight, currentFront));

    arcballTarget += currentRight * (-middleButtonDelta.x * arcballPanSpeed);
    arcballTarget += currentUp * (-middleButtonDelta.y * arcballPanSpeed);

    position = arcballTarget + offsetDirection * arcballDistance;
    front = glm::normalize(arcballTarget - position);
    up = currentUp;
}

void Camera::update() {
    constexpr uint_fast8_t left = 0;
    constexpr uint_fast8_t right = 1;
    constexpr uint_fast8_t middle = 2;
    std::array<glm::vec2, 3> buttonDelta{};
    bool mousePositionChanged = false;
    static bool run = false;

    static bool cameraToggleDown = false;
    const bool toggleDown =
            glfwGetKey(Window::get().window, GLFW_KEY_C) == GLFW_PRESS;
    if (toggleDown && !cameraToggleDown) {
        toggleMode();
    }
    cameraToggleDown = toggleDown;

    for (uint_fast8_t i = 0; i < 3; ++i) {
        buttonDelta[i] = Window::get().mouse.buttonDown[i].position -
                                         Window::get().mouse.previousButtonDown[i].position;

        if (Window::get().mouse.buttonDown[i].position !=
                Window::get().mouse.previousButtonDown[i].position) {
            mousePositionChanged = true;
            Window::get().mouse.previousButtonDown[i].position =
                    Window::get().mouse.buttonDown[i].position;
        }
    }

    if (mousePositionChanged) {
        run = true;
    }

    if (!run) {
        return;
    }

    const glm::vec2 leftButtonDelta = buttonDelta[left];
    const glm::vec2 rightButtonDelta = buttonDelta[right];
    const glm::vec2 middleButtonDelta = buttonDelta[middle];

    if (mode == Mode::Arcball) {
        applyArcballMode(leftButtonDelta, rightButtonDelta, middleButtonDelta);
    } else {
        applyPanningMode(leftButtonDelta, rightButtonDelta);
    }

    run = mousePositionChanged;
}

glm::mat4 Camera::setModel() {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

glm::mat4 Camera::setView() {
    update();
    glm::mat4 view;
    view = glm::lookAt(position, position + front, up);
    return view;
}

glm::mat4 Camera::setProjection(const VkExtent2D& swapchainExtent) {
  glm::mat4 projection = glm::perspective(
      glm::radians(fieldOfView),
      swapchainExtent.width / static_cast<float>(swapchainExtent.height),
      nearClipping, farClipping);

  projection[1][1] *= -1;  // flip y axis
  projection[0][0] *= -1;  // flip x axis

  return projection;
};
