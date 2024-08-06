#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class Camera {
 public:
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
        position(pos) {}

  glm::mat4 setModel();
  glm::mat4 setView();
  glm::mat4 setProjection(const VkExtent2D& swapchainExtent);

 private:
  void update();
};
