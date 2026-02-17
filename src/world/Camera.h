#pragma once

// Camera transform and interaction model.
// Exists to encapsulate view/projection setup and user navigation behavior.
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class Camera {
public:
  enum class Mode { Panning, Arcball };

  Camera(float zoom, float pan, float fov, float near, float far, glm::vec3 pos)
      : zoom_speed(zoom), panning_speed(pan), field_of_view(fov), near_clipping(near),
        far_clipping(far), position(pos), mode(Mode::Panning),
        arcball_target(0.0f, 0.0f, 0.0f), arcball_distance(glm::length(pos)),
        arcball_min_distance(2.0f), arcball_max_distance(300.0f), arcball_yaw(0.0f),
        arcball_pitch(0.0f), arcball_rotate_speed(1.4f), arcball_pan_speed(pan),
        arcball_zoom_speed(zoom), arcball_tumble_mult(1.0f), arcball_pan_mult(1.0f),
        arcball_dolly_mult(1.0f), arcball_pan_scalar(0.5f), arcball_zoom_scalar(0.1f),
        arcball_smoothing(0.2f), arcball_distance_pan_scale(0.8f),
        arcball_distance_zoom_scale(0.6f),
        arcball_preset_reference_distance(glm::length(pos)), arcball_horizon_lock(true),
        arcball_use_configured_target(false), arcball_cursor_initialized(false),
        arcball_left_was_down(false), arcball_right_was_down(false),
        arcball_last_cursor(0.0f, 0.0f), arcball_smoothed_delta(0.0f, 0.0f) {}

  void set_mode(Mode new_mode) {
    mode = new_mode;
  }
  Mode get_mode() const {
    return mode;
  }
  void toggle_mode();
  void configure_arcball(const glm::vec3 &target, float scene_radius);
  void configure_arcball_multipliers(float tumble, float pan, float dolly);
  void configure_arcball_response(float smoothing,
                                  float pan_scalar,
                                  float zoom_scalar,
                                  float distance_pan_scale,
                                  float distance_zoom_scale);
  void set_arcball_horizon_lock(bool enabled) {
    arcball_horizon_lock = enabled;
  }
  void set_pose(const glm::vec3 &new_position,
                const glm::vec3 &look_at,
                const glm::vec3 &up_hint = glm::vec3(0.0f, 0.0f, 1.0f));
  void set_orbit_view(float yaw_degrees, float pitch_degrees, float distance_scale);
  void set_preset_view(uint32_t preset_index);
  bool get_arcball_horizon_lock() const {
    return arcball_horizon_lock;
  }

  glm::mat4 set_model();
  glm::mat4 set_view();
  glm::mat4 set_projection(const VkExtent2D &swapchain_extent);

private:
  float zoom_speed;
  float panning_speed;
  float field_of_view;
  float near_clipping;
  float far_clipping;
  glm::vec3 position;
  glm::vec3 front{0.0f, 1.0f, -0.35f};
  glm::vec3 up{0.0f, 0.0f, 1.0f};

  void apply_panning_mode(const glm::vec2 &left_button_delta,
                          const glm::vec2 &right_button_delta);
  void apply_arcball_mode(const glm::vec2 &previous_cursor,
                          const glm::vec2 &current_cursor,
                          const bool left_pressed,
                          const bool right_pressed,
                          const bool middle_pressed,
                          const float viewport_width,
                          const float viewport_height);
  glm::vec3 map_cursor_to_arcball(const glm::vec2 &cursor,
                                  const float viewport_width,
                                  const float viewport_height) const;
  void sync_arcball_from_current_view(bool keep_configured_target);

  Mode mode;
  glm::vec3 arcball_target;
  float arcball_distance;
  float arcball_min_distance;
  float arcball_max_distance;
  float arcball_yaw;
  float arcball_pitch;
  float arcball_rotate_speed;
  float arcball_pan_speed;
  float arcball_zoom_speed;
  float arcball_tumble_mult;
  float arcball_pan_mult;
  float arcball_dolly_mult;
  float arcball_pan_scalar;
  float arcball_zoom_scalar;
  float arcball_smoothing;
  float arcball_distance_pan_scale;
  float arcball_distance_zoom_scale;
  float arcball_preset_reference_distance;
  bool arcball_horizon_lock;
  bool arcball_use_configured_target;
  bool arcball_cursor_initialized;
  bool arcball_left_was_down;
  bool arcball_right_was_down;
  glm::vec2 arcball_last_cursor;
  glm::vec2 arcball_smoothed_delta;

  VkExtent2D cached_extent{0, 0};
  glm::mat4 cached_projection{};
  bool input_changed{true};

  void update();
};

struct ModelViewProjection {
  alignas(16) glm::mat4 model{};
  alignas(16) glm::mat4 view{};
  alignas(16) glm::mat4 projection{};
};

class Light {
public:
  glm::vec4 position;
  Light(glm::vec4 p) : position(p){};
};
