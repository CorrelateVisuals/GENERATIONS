#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Camera.h"
#include "core/Log.h"
#include "platform/Window.h"

#include <algorithm>
#include <array>
#include <cmath>

void Camera::configureArcball(const glm::vec3& target, float sceneRadius) {
    arcballTarget = target;
    arcballUseConfiguredTarget = true;

    const float safeRadius = std::max(sceneRadius, 0.1f);
    arcballMinDistance = std::max(safeRadius * 0.35f, 1.0f);
    arcballMaxDistance = safeRadius * 14.0f;
    arcballDistance = std::clamp(safeRadius * 2.8f, arcballMinDistance,
                                 arcballMaxDistance);
}

void Camera::toggleMode() {
    if (mode == Mode::Panning) {
        syncArcballFromCurrentView(arcballUseConfiguredTarget);
        mode = Mode::Arcball;
        Log::text("{ Cam }", "Mode: Arcball");
        return;
    }

    mode = Mode::Panning;
    Log::text("{ Cam }", "Mode: Panning");
}

void Camera::syncArcballFromCurrentView(bool keepConfiguredTarget) {
    if (!keepConfiguredTarget) {
        const glm::vec3 forward = glm::normalize(front);
        const float targetDistance =
            std::clamp(arcballDistance, arcballMinDistance, arcballMaxDistance);
        arcballTarget = position + forward * targetDistance;
    }

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

glm::vec3 Camera::mapCursorToArcball(const glm::vec2& cursor,
                                     const float viewportWidth,
                                     const float viewportHeight) const {
    const float safeWidth = std::max(viewportWidth, 1.0f);
    const float safeHeight = std::max(viewportHeight, 1.0f);

    float x = (2.0f * cursor.x - safeWidth) / safeWidth;
    float y = (safeHeight - 2.0f * cursor.y) / safeHeight;

    const float lengthSquared = x * x + y * y;
    if (lengthSquared > 1.0f) {
        const float invLength = 1.0f / std::sqrt(lengthSquared);
        return glm::vec3(x * invLength, y * invLength, 0.0f);
    }

    return glm::vec3(x, y, std::sqrt(1.0f - lengthSquared));
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

void Camera::applyArcballMode(const glm::vec2& previousCursor,
                              const glm::vec2& currentCursor,
                              const bool leftPressed,
                              const bool rightPressed,
                              const bool middlePressed,
                              const float viewportWidth,
                              const float viewportHeight) {
    const float safeHeight = std::max(viewportHeight, 1.0f);
    const glm::vec2 cursorDelta = currentCursor - previousCursor;

    if (leftPressed) {
        const glm::vec3 from = mapCursorToArcball(previousCursor, viewportWidth,
                                                  viewportHeight);
        const glm::vec3 to = mapCursorToArcball(currentCursor, viewportWidth,
                                                viewportHeight);

        const float dotValue = std::clamp(glm::dot(from, to), -1.0f, 1.0f);
        const float angle = std::acos(dotValue);
        const glm::vec3 axisCamera = glm::cross(from, to);

        if (glm::length2(axisCamera) > 1e-9f && angle > 1e-5f) {
            const glm::vec3 forward = glm::normalize(arcballTarget - position);
            const glm::vec3 worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
            const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
            const glm::vec3 upAxis = glm::normalize(glm::cross(right, forward));

            const glm::vec3 axisWorld = glm::normalize(
                axisCamera.x * right + axisCamera.y * upAxis + axisCamera.z * (-forward));

            const glm::quat rotation = glm::angleAxis(angle * arcballRotateSpeed, axisWorld);
            glm::vec3 offset = position - arcballTarget;
            offset = rotation * offset;
            position = arcballTarget + offset;
            up = glm::normalize(rotation * up);
        }
    }

    glm::vec3 viewFront = glm::normalize(front);
    glm::vec3 viewRight = glm::normalize(glm::cross(viewFront, up));
    glm::vec3 viewUp = glm::normalize(glm::cross(viewRight, viewFront));

    if (rightPressed) {
        const float viewScale =
            (2.0f * arcballDistance * std::tan(glm::radians(fieldOfView) * 0.5f)) /
            safeHeight;
        const float panScale = viewScale * arcballPanSpeed * 0.5f;
        const glm::vec3 translation =
            (-cursorDelta.x * panScale) * viewRight +
            (-cursorDelta.y * panScale) * viewUp;
        position += translation;
        arcballTarget += translation;
    }

    if (middlePressed) {
        arcballDistance += cursorDelta.y * arcballZoomSpeed * 0.1f;
        arcballDistance = std::clamp(arcballDistance, arcballMinDistance,
                                     arcballMaxDistance);
        position = arcballTarget - viewFront * arcballDistance;
    }

    if (leftPressed) {
        position = arcballTarget + glm::normalize(position - arcballTarget) * arcballDistance;
    }

    front = glm::normalize(arcballTarget - position);
    viewRight = glm::normalize(glm::cross(front, viewUp));
    up = glm::normalize(glm::cross(viewRight, front));
    arcballDistance = glm::length(arcballTarget - position);
}

void Camera::update() {
    constexpr uint_fast8_t left = 0;
    constexpr uint_fast8_t right = 1;
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

    if (mode == Mode::Arcball) {
        double xpos(0.0), ypos(0.0);
        glfwGetCursorPos(Window::get().window, &xpos, &ypos);
        const glm::vec2 cursorPos(static_cast<float>(xpos),
                                  static_cast<float>(ypos));

        const bool leftPressed =
            glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_LEFT) ==
            GLFW_PRESS;
        const bool rightPressed =
            glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_RIGHT) ==
            GLFW_PRESS;
        const bool middlePressed =
            glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_MIDDLE) ==
            GLFW_PRESS;

        if (!arcballCursorInitialized) {
            arcballLastCursor = cursorPos;
            arcballCursorInitialized = true;
        }

        if ((!arcballLeftWasDown && leftPressed) ||
            (!arcballRightWasDown && rightPressed)) {
            arcballLastCursor = cursorPos;
        }

        const glm::vec2 previousCursor = arcballLastCursor;
        arcballLastCursor = cursorPos;

        applyArcballMode(previousCursor, cursorPos, leftPressed, rightPressed,
                 middlePressed,
                 static_cast<float>(Window::get().display.width),
                 static_cast<float>(Window::get().display.height));

        arcballLeftWasDown = leftPressed;
        arcballRightWasDown = rightPressed;
        return;
    }

    if (mousePositionChanged) {
        run = true;
    }

    if (!run) {
        return;
    } else {
        const glm::vec2 leftButtonDelta = buttonDelta[left];
        const glm::vec2 rightButtonDelta = buttonDelta[right];
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
