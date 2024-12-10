#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>

#include "Camera.h"
#include "Window.h"

void Camera::update() {
    glm::vec2 buttonType[3]{};
    constexpr uint_fast8_t left = 0;
    constexpr uint_fast8_t right = 1;
    constexpr uint_fast8_t middle = 2;
    bool mousePositionChanged = false;
    static bool run = false;

    for (uint_fast8_t i = 0; i < 3; ++i) {
        buttonType[i] = Window::get().mouse.buttonDown[i].position -
            Window::get().mouse.previousButtonDown[i].position;

        if (Window::get().mouse.buttonDown[i].position !=
            Window::get().mouse.previousButtonDown[i].position) {
            mousePositionChanged = true;
            Window::get().mouse.previousButtonDown[i].position =
                Window::get().mouse.buttonDown[i].position;
        }
    }
    if (mousePositionChanged) {
        run = mousePositionChanged;
    }

    if (run) {
        glm::vec2 leftButtonDelta = buttonType[left];
        glm::vec2 rightButtonDelta = buttonType[right];
        glm::vec2 middleButtonDelta = buttonType[middle];
        glm::vec3 cameraRight = glm::cross(front, up);

        glm::vec3 cameraUp = glm::cross(cameraRight, front);
        position -= panningSpeed * leftButtonDelta.x * cameraRight;
        position -= panningSpeed * leftButtonDelta.y * cameraUp;

        position += zoomSpeed * rightButtonDelta.x * front;
        position.z = std::max(position.z, 0.0f);
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

// void World::updateCamera() {
//     glm::vec2 buttonType[3]{};
//     constexpr uint_fast8_t left = 0;
//     constexpr uint_fast8_t right = 1;
//     constexpr uint_fast8_t middle = 2;
//     bool mousePositionChanged = false;
//     static bool run = false;
//
//     for (uint_fast8_t i = 0; i < 3; ++i) {
//         buttonType[i] = Window::get().mouse.buttonDown[i].position -
//             Window::get().mouse.previousButtonDown[i].position;
//
//         if (Window::get().mouse.buttonDown[i].position !=
//             Window::get().mouse.previousButtonDown[i].position) {
//             mousePositionChanged = true;
//             Window::get().mouse.previousButtonDown[i].position =
//                 Window::get().mouse.buttonDown[i].position;
//         }
//     }
//     if (mousePositionChanged) {
//         run = mousePositionChanged;
//     }
//
//     if (run) {
//         glm::vec2 leftButtonDelta = buttonType[left];
//         glm::vec2 rightButtonDelta = buttonType[right];
//         glm::vec2 middleButtonDelta = buttonType[middle];
//         glm::vec3 cameraRight = glm::cross(_camera.front, _camera.up);
//
//          glm::vec2 absDelta = glm::abs(leftButtonDelta);
//          constexpr float rotationSpeed = 0.4f * glm::pi<float>() / 180.0f;
//          glm::vec2 rotationDelta = rotationSpeed * leftButtonDelta;
//          glm::mat4 rotationMatrix(1.0f);
//
//          if (absDelta.y > absDelta.x) {
//            rotationMatrix =
//                glm::rotate(rotationMatrix, -rotationDelta.y, cameraRight);
//          } else if (absDelta.x > absDelta.y) {
//            rotationMatrix = glm::rotate(rotationMatrix, rotationDelta.x,
//            _camera.up);
//          }
//          _camera.front = glm::normalize(
//              glm::vec3(rotationMatrix * glm::vec4(_camera.front, 0.0f)));
//          _camera.up =
//              glm::normalize(glm::vec3(rotationMatrix * glm::vec4(_camera.up,
//              0.0f)));
//
//            float movementSpeed = getForwardMovement(leftButtonDelta);
//            _camera.position += movementSpeed * _camera.front;
//         constexpr float panningSpeed = 1.3f;
//         glm::vec3 cameraUp = glm::cross(cameraRight, _camera.front);
//         _camera.position -= panningSpeed * rightButtonDelta.x * cameraRight;
//         _camera.position -= panningSpeed * rightButtonDelta.y * cameraUp;
//
//         constexpr float zoomSpeed = 0.5f;
//         _camera.position += zoomSpeed * middleButtonDelta.x * _camera.front;
//         _camera.position.z = std::max(_camera.position.z, 0.0f);
//     }
//     run = mousePositionChanged;
// }

// float World::getForwardMovement(const glm::vec2& leftButtonDelta) {
//   static bool leftMouseButtonDown = false;
//   static float forwardMovement = 0.0f;
//   float leftButtonDeltaLength = glm::length(leftButtonDelta);
//
//   if (leftButtonDeltaLength > 0.0f) {
//     if (!leftMouseButtonDown) {
//       leftMouseButtonDown = true;
//       forwardMovement = 0.0f;
//     }
//     constexpr float maxSpeed = 0.1f;
//     constexpr float acceleration = 0.01f;
//
//     // Calculate the speed based on the distance from the center
//     float normalizedDeltaLength = glm::clamp(leftButtonDeltaLength,
//     0.0f, 1.0f); float targetSpeed =
//         glm::smoothstep(0.0f, maxSpeed, 1.0f - normalizedDeltaLength);
//     forwardMovement += acceleration * (targetSpeed - forwardMovement);
//     forwardMovement = glm::clamp(forwardMovement, 0.0f, maxSpeed);
//   } else {
//     leftMouseButtonDown = false;
//     forwardMovement = 0.0f;
//   }
//   return forwardMovement;
// }
