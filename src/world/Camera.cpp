#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include "Camera.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "platform/Window.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

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
  arcball_preset_reference_distance = arcball_distance;
}

void Camera::configure_arcball_multipliers(float tumble, float pan, float dolly) {
  arcball_tumble_mult = std::max(tumble, 0.01f);
  arcball_pan_mult = std::max(pan, 0.01f);
  arcball_dolly_mult = std::max(dolly, 0.01f);
}

void Camera::configure_arcball_response(float smoothing,
                                        float pan_scalar,
                                        float zoom_scalar,
                                        float distance_pan_scale,
                                        float distance_zoom_scale) {
  arcball_smoothing = std::clamp(smoothing, 0.0f, 1.0f);
  arcball_pan_scalar = std::max(pan_scalar, 0.01f);
  arcball_zoom_scalar = std::max(zoom_scalar, 0.01f);
  arcball_distance_pan_scale = std::max(distance_pan_scale, 0.0f);
  arcball_distance_zoom_scale = std::max(distance_zoom_scale, 0.0f);
}

void Camera::set_pose(const glm::vec3 &new_position,
                      const glm::vec3 &look_at,
                      const glm::vec3 &up_hint) {
  position = new_position;

  const glm::vec3 look_vector = look_at - new_position;
  if (glm::length2(look_vector) > 1e-10f) {
    front = glm::normalize(look_vector);
  }

  glm::vec3 right_axis = glm::cross(front, up_hint);
  if (glm::length2(right_axis) > 1e-10f) {
    right_axis = glm::normalize(right_axis);
    up = glm::normalize(glm::cross(right_axis, front));
  }

  arcball_target = look_at;
  arcball_use_configured_target = true;
  sync_arcball_from_current_view(true);
}

void Camera::set_orbit_view(float yaw_degrees,
                            float pitch_degrees,
                            float distance_scale) {
  mode = Mode::Arcball;

  const float base_distance =
      std::clamp(arcball_preset_reference_distance > 0.0f
                     ? arcball_preset_reference_distance
                     : (arcball_distance > 0.0f ? arcball_distance : arcball_min_distance * 1.8f),
                 arcball_min_distance,
                 arcball_max_distance);

  const float yaw = glm::radians(yaw_degrees);
  const float pitch = glm::radians(std::clamp(pitch_degrees, -89.0f, 89.0f));
  const float distance = std::clamp(base_distance * distance_scale,
                                    arcball_min_distance,
                                    arcball_max_distance);

  const glm::vec3 orbit_dir{std::cos(pitch) * std::cos(yaw),
                            std::cos(pitch) * std::sin(yaw),
                            std::sin(pitch)};
  set_pose(arcball_target + orbit_dir * distance, arcball_target);

  arcball_cursor_initialized = false;
  arcball_left_was_down = false;
  arcball_right_was_down = false;
}

void Camera::set_preset_view(uint32_t preset_index) {
  mode = Mode::Arcball;

  switch (preset_index) {
    case 1:
      set_orbit_view(28.0f, 28.0f, 0.74f);
      Log::text("{ Cam }", "Preset 1: Close Low Angle");
      break;
    case 2:
      set_orbit_view(0.0f, 12.0f, 0.78f);
      Log::text("{ Cam }", "Preset 2: Close Front Straight");
      break;
    case 3:
      set_orbit_view(90.0f, 12.0f, 0.78f);
      Log::text("{ Cam }", "Preset 3: Close Side Straight");
      break;
    case 4:
      set_orbit_view(10.0f, 87.0f, 0.75f);
      Log::text("{ Cam }", "Preset 4: Top Down");
      break;
    default:
      break;
  }
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
  const glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
  constexpr float dead_zone = 0.0004f;
  constexpr float response_max_drag = 0.035f;
  constexpr float pole_limit = 0.96f;
  const float safe_height = std::max(viewport_height, 1.0f);
  const float safe_width = std::max(viewport_width, 1.0f);
  const float safe_min_axis = std::max(std::min(safe_width, safe_height), 1.0f);
  const glm::vec2 cursor_delta = current_cursor - previous_cursor;
  const bool any_pressed = left_pressed || right_pressed || middle_pressed;
  if (!any_pressed) {
    arcball_smoothed_delta = glm::vec2(0.0f, 0.0f);
  }

  const float smoothing_factor = std::clamp(arcball_smoothing, 0.0f, 1.0f);
  const glm::vec2 smoothed_delta =
      glm::mix(arcball_smoothed_delta, cursor_delta, smoothing_factor);
  arcball_smoothed_delta = smoothed_delta;

  if (left_pressed) {
    const glm::vec2 normalized_delta = smoothed_delta / safe_min_axis;
    const float drag_magnitude = glm::length(normalized_delta);

    if (drag_magnitude > dead_zone) {
      const float response = glm::smoothstep(dead_zone, response_max_drag, drag_magnitude);
      const float yawAngle =
        normalized_delta.x * arcball_rotate_speed * arcball_tumble_mult * response;
      const float pitchAngle =
        -normalized_delta.y * arcball_rotate_speed * arcball_tumble_mult * response;

      glm::vec3 orbitOffset = position - arcball_target;

      // Compute right axis from current camera state BEFORE any rotation.
      // Using worldUp fails when looking straight down (front ≈ worldUp).
      glm::vec3 rightAxis = glm::cross(front, up);
      if (glm::length2(rightAxis) < 1e-10f) {
        rightAxis = glm::cross(front, worldUp);
      }
      if (glm::length2(rightAxis) < 1e-10f) {
        rightAxis = glm::vec3(1.0f, 0.0f, 0.0f);
      }
      rightAxis = glm::normalize(rightAxis);

      // Apply pitch first (around camera right axis)
      const glm::quat pitchRotation = glm::angleAxis(pitchAngle, rightAxis);
      glm::vec3 pitchedOffset = pitchRotation * orbitOffset;
      const glm::vec3 pitchedFront = glm::normalize(-pitchedOffset);

      // Allow pitch that moves away from the pole; block only deeper into it.
      const float currentPoleDot = std::abs(glm::dot(glm::normalize(-orbitOffset), worldUp));
      const float pitchedPoleDot = std::abs(glm::dot(pitchedFront, worldUp));
      if (pitchedPoleDot < pole_limit || pitchedPoleDot <= currentPoleDot) {
        orbitOffset = pitchedOffset;
      }

      // Apply yaw (around world up axis)
      const glm::quat yawRotation = glm::angleAxis(yawAngle, worldUp);
      orbitOffset = yawRotation * orbitOffset;

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
    const float distance_ratio = arcball_distance /
      std::max(arcball_preset_reference_distance, 0.001f);
    const float pan_distance_scale = std::clamp(
      1.0f + (distance_ratio - 1.0f) * arcball_distance_pan_scale, 0.25f, 3.5f);
    const float panScale =
      viewScale * arcball_pan_speed * arcball_pan_scalar * arcball_pan_mult *
      pan_distance_scale;
    const glm::vec3 translation =
      (-smoothed_delta.x * panScale) * viewRight +
      (-smoothed_delta.y * panScale) * viewUp;
    position += translation;
    arcball_target += translation;
  }

  if (middle_pressed) {
    const float distance_ratio = arcball_distance /
      std::max(arcball_preset_reference_distance, 0.001f);
    const float dolly_distance_scale = std::clamp(
      1.0f + (distance_ratio - 1.0f) * arcball_distance_zoom_scale, 0.25f, 3.5f);
    // Exponential zoom: multiply distance by a factor instead of adding
    // linearly. This gives natural-feeling zoom (fast when far, slow when close).
    const float zoom_input =
      smoothed_delta.y * arcball_zoom_speed * arcball_zoom_scalar *
      arcball_dolly_mult * dolly_distance_scale;
    const float zoom_factor = std::exp(zoom_input * 0.01f);
    arcball_distance *= zoom_factor;
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
  static bool camera_toggle_down = false;
  static bool horizon_toggle_down = false;
  static bool tuning_enabled = CE::Runtime::env_flag_enabled("CE_CAMERA_TUNING");
  static bool tuning_mode = false;
  static bool tuning_toggle_down = false;
  static bool tuning_prev_down = false;
  static bool tuning_next_down = false;
  static bool tuning_decrease_down = false;
  static bool tuning_increase_down = false;
  static int tuning_index = 0;
  static std::array<bool, 4> preset_toggle_down{false, false, false, false};

  const std::array<std::pair<int, uint32_t>, 4> preset_keys{{
      {GLFW_KEY_1, 1},
      {GLFW_KEY_2, 2},
      {GLFW_KEY_3, 3},
      {GLFW_KEY_4, 4},
  }};

  for (size_t i = 0; i < preset_keys.size(); ++i) {
    const bool preset_down =
        glfwGetKey(Window::get().window, preset_keys[i].first) == GLFW_PRESS;
    if (preset_down && !preset_toggle_down[i]) {
      set_preset_view(preset_keys[i].second);
      input_changed = true;
    }
    preset_toggle_down[i] = preset_down;
  }

  const bool toggle_down = glfwGetKey(Window::get().window, GLFW_KEY_C) == GLFW_PRESS;
  if (toggle_down && !camera_toggle_down) {
    toggle_mode();
    input_changed = true;
  }
  camera_toggle_down = toggle_down;

  const bool horizon_down = glfwGetKey(Window::get().window, GLFW_KEY_V) == GLFW_PRESS;
  if (horizon_down && !horizon_toggle_down) {
    arcball_horizon_lock = !arcball_horizon_lock;
    Log::text("{ Cam }",
              arcball_horizon_lock ? "Horizon Lock: On" : "Horizon Lock: Off");
    input_changed = true;
  }
  horizon_toggle_down = horizon_down;

  if (tuning_enabled) {
    const bool tuning_toggle = glfwGetKey(Window::get().window, GLFW_KEY_T) == GLFW_PRESS;
    if (tuning_toggle && !tuning_toggle_down) {
      tuning_mode = !tuning_mode;
      Log::text("{ Cam }", tuning_mode ? "Tuning: On" : "Tuning: Off");
    }
    tuning_toggle_down = tuning_toggle;
  } else {
    tuning_mode = false;
    tuning_toggle_down = false;
  }

  if (tuning_enabled && tuning_mode) {
    static const std::array<const char *, 11> tuning_labels = {
        "FOV",
        "Near Clip",
        "Far Clip",
        "Arcball Smoothing",
        "Pan Scalar",
        "Zoom Scalar",
        "Distance Pan",
        "Distance Zoom",
        "Tumble Mult",
        "Pan Mult",
        "Dolly Mult",
    };

    auto log_tuning_value = [&](const char *label, float value) {
      std::ostringstream stream;
      stream << std::fixed << std::setprecision(3) << value;
      Log::text("{ Cam }", std::string("Tune ") + label + " = " + stream.str());
    };

    auto get_value = [&]() -> float {
      switch (tuning_index) {
        case 0: return field_of_view;
        case 1: return near_clipping;
        case 2: return far_clipping;
        case 3: return arcball_smoothing;
        case 4: return arcball_pan_scalar;
        case 5: return arcball_zoom_scalar;
        case 6: return arcball_distance_pan_scale;
        case 7: return arcball_distance_zoom_scale;
        case 8: return arcball_tumble_mult;
        case 9: return arcball_pan_mult;
        case 10: return arcball_dolly_mult;
        default: return 0.0f;
      }
    };

    auto apply_delta = [&](float delta) {
      switch (tuning_index) {
        case 0:
          field_of_view = std::clamp(field_of_view + delta, 20.0f, 90.0f);
          break;
        case 1:
          near_clipping = std::clamp(near_clipping + delta, 0.01f, far_clipping - 0.05f);
          break;
        case 2:
          far_clipping = std::max(far_clipping + delta, near_clipping + 0.1f);
          break;
        case 3:
          arcball_smoothing = std::clamp(arcball_smoothing + delta, 0.0f, 1.0f);
          break;
        case 4:
          arcball_pan_scalar = std::max(arcball_pan_scalar + delta, 0.01f);
          break;
        case 5:
          arcball_zoom_scalar = std::max(arcball_zoom_scalar + delta, 0.01f);
          break;
        case 6:
          arcball_distance_pan_scale = std::clamp(arcball_distance_pan_scale + delta, 0.0f, 2.5f);
          break;
        case 7:
          arcball_distance_zoom_scale = std::clamp(arcball_distance_zoom_scale + delta, 0.0f, 2.5f);
          break;
        case 8:
          arcball_tumble_mult = std::max(arcball_tumble_mult + delta, 0.01f);
          break;
        case 9:
          arcball_pan_mult = std::max(arcball_pan_mult + delta, 0.01f);
          break;
        case 10:
          arcball_dolly_mult = std::max(arcball_dolly_mult + delta, 0.01f);
          break;
      }
      input_changed = true;
      log_tuning_value(tuning_labels[tuning_index], get_value());
    };

    const bool tuning_prev = glfwGetKey(Window::get().window, GLFW_KEY_COMMA) == GLFW_PRESS;
    if (tuning_prev && !tuning_prev_down) {
      tuning_index = (tuning_index + static_cast<int>(tuning_labels.size()) - 1) %
                     static_cast<int>(tuning_labels.size());
      log_tuning_value(tuning_labels[tuning_index], get_value());
    }
    tuning_prev_down = tuning_prev;

    const bool tuning_next = glfwGetKey(Window::get().window, GLFW_KEY_PERIOD) == GLFW_PRESS;
    if (tuning_next && !tuning_next_down) {
      tuning_index = (tuning_index + 1) % static_cast<int>(tuning_labels.size());
      log_tuning_value(tuning_labels[tuning_index], get_value());
    }
    tuning_next_down = tuning_next;

    const bool tuning_decrease =
        glfwGetKey(Window::get().window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
    if (tuning_decrease && !tuning_decrease_down) {
      const float step = (tuning_index == 0 || tuning_index == 2) ? 1.0f :
                         (tuning_index == 1 ? 0.02f : 0.05f);
      apply_delta(-step);
    }
    tuning_decrease_down = tuning_decrease;

    const bool tuning_increase =
        glfwGetKey(Window::get().window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS;
    if (tuning_increase && !tuning_increase_down) {
      const float step = (tuning_index == 0 || tuning_index == 2) ? 1.0f :
                         (tuning_index == 1 ? 0.02f : 0.05f);
      apply_delta(step);
    }
    tuning_increase_down = tuning_increase;
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

  double xpos(0.0), ypos(0.0);
  glfwGetCursorPos(Window::get().window, &xpos, &ypos);
  const glm::vec2 cursor_pos(static_cast<float>(xpos), static_cast<float>(ypos));

  const bool left_pressed =
      glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  const bool right_pressed =
      glfwGetMouseButton(Window::get().window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

  static bool panning_cursor_initialized = false;
  static bool panning_left_was_down = false;
  static bool panning_right_was_down = false;
  static glm::vec2 panning_last_cursor(0.0f, 0.0f);
  static glm::vec2 panning_left_hold_direction(0.0f, 0.0f);
  static glm::vec2 panning_right_hold_direction(0.0f, 0.0f);

  if (!panning_cursor_initialized) {
    panning_last_cursor = cursor_pos;
    panning_cursor_initialized = true;
  }

  if ((!panning_left_was_down && left_pressed) ||
      (!panning_right_was_down && right_pressed)) {
    panning_last_cursor = cursor_pos;
    if (!panning_left_was_down && left_pressed) {
      panning_left_hold_direction = glm::vec2(0.0f, 0.0f);
    }
    if (!panning_right_was_down && right_pressed) {
      panning_right_hold_direction = glm::vec2(0.0f, 0.0f);
    }
  }

  const glm::vec2 cursor_delta = cursor_pos - panning_last_cursor;
  panning_last_cursor = cursor_pos;

  if (left_pressed || right_pressed) {
    constexpr float panning_drag_gain = 8.0f;
    constexpr float panning_left_hold_speed = 5.5f;
    constexpr float panning_right_hold_speed = 14.0f;
    const float safe_min_axis =
        std::max(1.0f,
                 std::min(static_cast<float>(Window::get().display.width),
                          static_cast<float>(Window::get().display.height)));
    const glm::vec2 normalized_delta = (cursor_delta / safe_min_axis) * panning_drag_gain;

    if (left_pressed && glm::length(normalized_delta) > 0.0001f) {
      panning_left_hold_direction = glm::normalize(normalized_delta);
    }

    if (right_pressed && glm::length(normalized_delta) > 0.0001f) {
      panning_right_hold_direction = glm::normalize(normalized_delta);
    }

    const glm::vec2 left_button_delta =
        left_pressed ? panning_left_hold_direction * panning_left_hold_speed : glm::vec2(0.0f);
    const glm::vec2 right_button_delta =
        right_pressed ? panning_right_hold_direction * panning_right_hold_speed : glm::vec2(0.0f);
    apply_panning_mode(left_button_delta, right_button_delta);
  }

  // WASD keyboard support
  constexpr float keyboard_pan_speed = 7.0f;
  glm::vec2 keyboard_delta(0.0f);
  
  if (glfwGetKey(Window::get().window, GLFW_KEY_W) == GLFW_PRESS) {
    keyboard_delta.y -= keyboard_pan_speed;
  }
  if (glfwGetKey(Window::get().window, GLFW_KEY_S) == GLFW_PRESS) {
    keyboard_delta.y += keyboard_pan_speed;
  }
  if (glfwGetKey(Window::get().window, GLFW_KEY_A) == GLFW_PRESS) {
    keyboard_delta.x -= keyboard_pan_speed;
  }
  if (glfwGetKey(Window::get().window, GLFW_KEY_D) == GLFW_PRESS) {
    keyboard_delta.x += keyboard_pan_speed;
  }
  
  if (glm::length(keyboard_delta) > 0.0001f) {
    apply_panning_mode(keyboard_delta, glm::vec2(0.0f));
    input_changed = true;
  }

  panning_left_was_down = left_pressed;
  panning_right_was_down = right_pressed;
}

glm::mat4 Camera::set_model() {
  static const glm::mat4 identity = glm::mat4(1.0f);
  return identity;
}

glm::mat4 Camera::set_view() {
  update();
  glm::mat4 view = glm::lookAt(position, position + front, up);
  return view;
}

glm::mat4 Camera::set_projection(const VkExtent2D &swapchain_extent) {
  if (cached_extent.width == swapchain_extent.width &&
      cached_extent.height == swapchain_extent.height &&
      cached_extent.width != 0) {
    return cached_projection;
  }

  cached_extent = swapchain_extent;
  glm::mat4 projection =
    glm::perspective(glm::radians(field_of_view),
             swapchain_extent.width /
               static_cast<float>(swapchain_extent.height),
             near_clipping,
             far_clipping);

  projection[1][1] *= -1; // flip y axis
  projection[0][0] *= -1; // flip x axis

  cached_projection = projection;
  return cached_projection;
};
