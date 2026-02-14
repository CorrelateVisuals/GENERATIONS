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

void Camera::configure_arcball(const glm::vec3 &target, float scene_radius) {
  arcball_target = target;
  arcball_use_configured_target = true;

  const float safe_radius = std::max(scene_radius, 0.1f);
  constexpr float min_distance_radius_scale = 0.35f;
  constexpr float max_distance_radius_scale = 14.0f;
  constexpr float default_distance_radius_scale = 2.8f;
  arcball_min_distance = std::max(safe_radius * min_distance_radius_scale, 1.0f);
  arcball_max_distance = safe_radius * max_distance_radius_scale;
  arcball_distance = std::clamp(safe_radius * default_distance_radius_scale,
                                arcball_min_distance,
                                arcball_max_distance);
}

void Camera::configure_arcball_multipliers(float tumble, float pan, float dolly) {
  arcball_tumble_mult = std::max(tumble, 0.01f);
  arcball_pan_mult = std::max(pan, 0.01f);
  arcball_dolly_mult = std::max(dolly, 0.01f);
}

void Camera::toggle_mode() {
  if (mode == Mode::Panning) {
    sync_arcball_from_current_view(arcball_use_configured_target);
    mode = Mode::Arcball;
    Log::text("{ Cam }", "Mode: Arcball");
    return;
  }

  mode = Mode::Panning;
  Log::text("{ Cam }", "Mode: Panning");
}

void Camera::sync_arcball_from_current_view(bool keep_configured_target) {
  if (!keep_configured_target) {
    const glm::vec3 forward = glm::normalize(front);
    const float target_distance =
        std::clamp(arcball_distance, arcball_min_distance, arcball_max_distance);
    arcball_target = position + forward * target_distance;
  }

  arcball_distance = glm::length(arcball_target - position);
  if (arcball_distance <= 0.0001f) {
    arcball_distance = arcball_min_distance;
  }

  const glm::vec3 offset = position - arcball_target;
  const float horizontal = std::sqrt(offset.x * offset.x + offset.y * offset.y);
  arcball_yaw = std::atan2(offset.y, offset.x);
  arcball_pitch = std::atan2(offset.z, std::max(horizontal, 0.0001f));
  arcball_pitch = std::clamp(arcball_pitch, glm::radians(-89.0f), glm::radians(89.0f));
}

glm::vec3 Camera::map_cursor_to_arcball(const glm::vec2 &cursor,
                                        const float viewport_width,
                                        const float viewport_height) const {
  const float safe_width = std::max(viewport_width, 1.0f);
  const float safe_height = std::max(viewport_height, 1.0f);

  float x = (2.0f * cursor.x - safe_width) / safe_width;
  float y = (safe_height - 2.0f * cursor.y) / safe_height;

  const float lengthSquared = x * x + y * y;
  if (lengthSquared > 1.0f) {
    const float invLength = 1.0f / std::sqrt(lengthSquared);
    return glm::vec3(x * invLength, y * invLength, 0.0f);
  }

  return glm::vec3(x, y, std::sqrt(1.0f - lengthSquared));
}

void Camera::apply_panning_mode(const glm::vec2 &left_button_delta,
                                const glm::vec2 &right_button_delta) {
  glm::vec3 camera_right = glm::normalize(glm::cross(front, up));
  glm::vec3 camera_up = glm::normalize(glm::cross(camera_right, front));

  position -= panning_speed * left_button_delta.x * camera_right;
  position -= panning_speed * left_button_delta.y * camera_up;

  position += zoom_speed * right_button_delta.x * front;
  position.z = std::max(position.z, 0.0f);
}

void Camera::apply_arcball_mode(const glm::vec2 &previous_cursor,
                                const glm::vec2 &current_cursor,
                                const bool left_pressed,
                                const bool right_pressed,
                                const bool middle_pressed,
                                const float viewport_width,
                                const float viewport_height) {
  const glm::vec3 worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
  constexpr float arcball_pan_scalar = 0.5f;
  constexpr float arcball_zoom_scalar = 0.1f;
  constexpr float dead_zone = 0.0008f;
  constexpr float response_max_drag = 0.06f;
  constexpr float pole_limit = 0.985f;
  const float safe_height = std::max(viewport_height, 1.0f);
  const float safe_width = std::max(viewport_width, 1.0f);
  const float safe_min_axis = std::max(std::min(safe_width, safe_height), 1.0f);
  const glm::vec2 cursor_delta = current_cursor - previous_cursor;

  if (left_pressed) {
    const glm::vec2 normalized_delta = cursor_delta / safe_min_axis;
    const float drag_magnitude = glm::length(normalized_delta);

    if (drag_magnitude > dead_zone) {
      const float response = glm::smoothstep(dead_zone, response_max_drag, drag_magnitude);
      const float yawAngle =
          -normalized_delta.x * arcball_rotate_speed * arcball_tumble_mult * response;
      const float pitchAngle =
          normalized_delta.y * arcball_rotate_speed * arcball_tumble_mult * response;

      glm::vec3 orbitOffset = position - arcball_target;
      const glm::quat yawRotation = glm::angleAxis(yawAngle, worldUp);
      orbitOffset = yawRotation * orbitOffset;

      const glm::vec3 frontAfterYaw = glm::normalize(-orbitOffset);
      glm::vec3 rightAxis = glm::cross(frontAfterYaw, worldUp);
      if (glm::length2(rightAxis) > 1e-10f) {
        rightAxis = glm::normalize(rightAxis);

        const glm::quat pitchRotation = glm::angleAxis(pitchAngle, rightAxis);
        const glm::vec3 pitchedOffset = pitchRotation * orbitOffset;
        const glm::vec3 pitchedFront = glm::normalize(-pitchedOffset);

        if (std::abs(glm::dot(pitchedFront, worldUp)) < pole_limit) {
          orbitOffset = pitchedOffset;
        }
      }

      position = arcball_target + orbitOffset;
    }
  }

  glm::vec3 viewFront = glm::normalize(front);
  glm::vec3 viewRight = glm::normalize(glm::cross(viewFront, up));
  glm::vec3 viewUp = glm::normalize(glm::cross(viewRight, viewFront));

  if (right_pressed) {
    const float viewScale =
        (2.0f * arcball_distance * std::tan(glm::radians(field_of_view) * 0.5f)) /
        safe_height;
    const float panScale =
        viewScale * arcball_pan_speed * arcball_pan_scalar * arcball_pan_mult;
    const glm::vec3 translation =
        (-cursor_delta.x * panScale) * viewRight + (-cursor_delta.y * panScale) * viewUp;
    position += translation;
    arcball_target += translation;
  }

  if (middle_pressed) {
    arcball_distance +=
        cursor_delta.y * arcball_zoom_speed * arcball_zoom_scalar * arcball_dolly_mult;
    arcball_distance =
        std::clamp(arcball_distance, arcball_min_distance, arcball_max_distance);
    position = arcball_target - viewFront * arcball_distance;
  }

  if (left_pressed) {
    position = arcball_target +
               glm::normalize(position - arcball_target) * arcball_distance;
  }

  front = glm::normalize(arcball_target - position);
  if (arcball_horizon_lock) {
    viewRight = glm::cross(front, worldUp);
    if (glm::length2(viewRight) > 1e-10f) {
      viewRight = glm::normalize(viewRight);
      up = glm::normalize(glm::cross(viewRight, front));
    } else {
      up = viewUp;
    }
  } else {
    viewRight = glm::normalize(glm::cross(front, viewUp));
    up = glm::normalize(glm::cross(viewRight, front));
  }
  arcball_distance = glm::length(arcball_target - position);
}

void Camera::update() {
  constexpr uint_fast8_t left = 0;
  constexpr uint_fast8_t right = 1;
  std::array<glm::vec2, 3> buttonDelta{};
  bool mousePositionChanged = false;
  static bool run = false;

  static bool camera_toggle_down = false;
  static bool horizon_toggle_down = false;
  const bool toggle_down = glfwGetKey(Window::get().window, GLFW_KEY_C) == GLFW_PRESS;
  if (toggle_down && !camera_toggle_down) {
    toggle_mode();
  }
  camera_toggle_down = toggle_down;

  const bool horizon_down = glfwGetKey(Window::get().window, GLFW_KEY_V) == GLFW_PRESS;
  if (horizon_down && !horizon_toggle_down) {
    arcball_horizon_lock = !arcball_horizon_lock;
    Log::text("{ Cam }",
              arcball_horizon_lock ? "Horizon Lock: On" : "Horizon Lock: Off");
  }
  horizon_toggle_down = horizon_down;

  for (uint_fast8_t i = 0; i < 3; ++i) {
    buttonDelta[i] = Window::get().mouse.button_down[i].position -
                     Window::get().mouse.previous_button_down[i].position;

    if (Window::get().mouse.button_down[i].position !=
        Window::get().mouse.previous_button_down[i].position) {
      mousePositionChanged = true;
      Window::get().mouse.previous_button_down[i].position =
          Window::get().mouse.button_down[i].position;
    }
  }

  if (mode == Mode::Arcball) {
    double xpos(0.0), ypos(0.0);
    glfwGetCursorPos(Window::get().window, &xpos, &ypos);
    const glm::vec2 cursor_pos(static_cast<float>(xpos), static_cast<float>(ypos));

    const bool left_pressed =
        glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool right_pressed =
        glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middle_pressed =
        glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    if (!arcball_cursor_initialized) {
      arcball_last_cursor = cursor_pos;
      arcball_cursor_initialized = true;
    }

    if ((!arcball_left_was_down && left_pressed) ||
        (!arcball_right_was_down && right_pressed)) {
      arcball_last_cursor = cursor_pos;
    }

    const glm::vec2 previous_cursor = arcball_last_cursor;
    arcball_last_cursor = cursor_pos;

    apply_arcball_mode(previous_cursor,
                     cursor_pos,
                     left_pressed,
                     right_pressed,
                     middle_pressed,
                     static_cast<float>(Window::get().display.width),
                     static_cast<float>(Window::get().display.height));

    arcball_left_was_down = left_pressed;
    arcball_right_was_down = right_pressed;
    return;
  }

  if (mousePositionChanged) {
    run = true;
  }

  if (!run) {
    return;
  } else {
    const glm::vec2 left_button_delta = buttonDelta[left];
    const glm::vec2 right_button_delta = buttonDelta[right];
    apply_panning_mode(left_button_delta, right_button_delta);
  }

  run = mousePositionChanged;
}

glm::mat4 Camera::set_model() {
  glm::mat4 model =
      glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  return model;
}

glm::mat4 Camera::set_view() {
  update();
  glm::mat4 view;
  view = glm::lookAt(position, position + front, up);
  return view;
}

glm::mat4 Camera::set_projection(const VkExtent2D &swapchain_extent) {
  glm::mat4 projection =
    glm::perspective(glm::radians(field_of_view),
             swapchain_extent.width /
               static_cast<float>(swapchain_extent.height),
             near_clipping,
             far_clipping);

  projection[1][1] *= -1; // flip y axis
  projection[0][0] *= -1; // flip x axis

  return projection;
};
