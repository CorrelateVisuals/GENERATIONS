#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class Camera {
 public:
  enum class Mode { Panning, Arcball };

  float zoomSpeed;
  float panningSpeed;
  float fieldOfView;
  float nearClipping;
  float farClipping;
  glm::vec3 position;
  glm::vec3 front{0.0f, 0.0f, -1.0f};
  glm::vec3 up{0.0f, -1.0f, 0.0f};

  Camera(float zoom, float pan, float fov, float near, float far, glm::vec3 pos)
      : zoomSpeed(zoom),
        panningSpeed(pan),
        fieldOfView(fov),
        nearClipping(near),
        farClipping(far),
        position(pos),
        mode(Mode::Panning),
        arcballTarget(0.0f, 0.0f, 0.0f),
        arcballDistance(glm::length(pos)),
        arcballMinDistance(2.0f),
        arcballMaxDistance(300.0f),
        arcballYaw(0.0f),
        arcballPitch(0.0f),
        arcballRotateSpeed(2.5f),
        arcballPanSpeed(pan),
        arcballZoomSpeed(zoom) {}

  void setMode(Mode newMode) { mode = newMode; }
  Mode getMode() const { return mode; }
  void toggleMode();

  glm::mat4 setModel();
  glm::mat4 setView();
  glm::mat4 setProjection(const VkExtent2D& swapchainExtent);

 private:
  void applyPanningMode(const glm::vec2& leftButtonDelta,
                        const glm::vec2& rightButtonDelta);
  void applyArcballMode(const glm::vec2& leftButtonDelta,
                        const glm::vec2& rightButtonDelta,
                        const glm::vec2& middleButtonDelta);
  void syncArcballFromCurrentView();

  Mode mode;
  glm::vec3 arcballTarget;
  float arcballDistance;
  float arcballMinDistance;
  float arcballMaxDistance;
  float arcballYaw;
  float arcballPitch;
  float arcballRotateSpeed;
  float arcballPanSpeed;
  float arcballZoomSpeed;

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
