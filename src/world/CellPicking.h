#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CellPicking {

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
};

struct GridPickResult {
  bool hit;
  int cellX;
  int cellY;
  int cellIndex;
  glm::vec3 worldPosition;
};

// Method 4: Grid-Based Picking (Optimized for Regular Grids)
// This is the recommended approach for the GENERATIONS cellular grid
inline Ray screen_to_world_ray(float screenX, float screenY, const glm::mat4 &view,
                                const glm::mat4 &projection, int screenWidth,
                                int screenHeight) {
  // Convert screen coords to NDC [-1, 1]
  float x = (2.0f * screenX) / static_cast<float>(screenWidth) - 1.0f;
  float y = 1.0f - (2.0f * screenY) / static_cast<float>(screenHeight);

  // NDC to world space
  glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
  glm::vec4 rayEye = glm::inverse(projection) * rayClip;
  rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
  glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);

  Ray ray;
  ray.origin = glm::vec3(glm::inverse(view) * glm::vec4(0, 0, 0, 1));
  ray.direction = glm::normalize(rayWorld);
  return ray;
}

inline GridPickResult pick_grid_cell(float mouseX, float mouseY, const glm::mat4 &view,
                                     const glm::mat4 &projection, int gridWidth,
                                     int gridHeight, float cellSize, int screenWidth,
                                     int screenHeight, float gridY = 0.0f) {
  // Create ray from camera through mouse
  Ray ray = screen_to_world_ray(mouseX, mouseY, view, projection, screenWidth, screenHeight);

  // Intersect with grid plane (y = gridY)
  GridPickResult result;
  result.hit = false;
  result.cellX = -1;
  result.cellY = -1;
  result.cellIndex = -1;

  // Check if ray is parallel to grid plane
  if (std::abs(ray.direction.y) < 0.0001f) {
    return result;
  }

  float t = (gridY - ray.origin.y) / ray.direction.y;

  // Check if intersection is behind camera
  if (t < 0.0f) {
    return result;
  }

  glm::vec3 hitPoint = ray.origin + ray.direction * t;
  result.worldPosition = hitPoint;

  // Convert world position to grid coordinates
  // Grid is centered at origin, so adjust accordingly
  int gridX = static_cast<int>((hitPoint.x / cellSize) + gridWidth / 2.0f);
  int gridZ = static_cast<int>((hitPoint.z / cellSize) + gridHeight / 2.0f);

  // Check if within grid bounds
  result.hit = (gridX >= 0 && gridX < gridWidth && gridZ >= 0 && gridZ < gridHeight);

  if (result.hit) {
    result.cellX = gridX;
    result.cellY = gridZ;
    result.cellIndex = gridZ * gridWidth + gridX;
  }

  return result;
}

// Method 2: CPU Ray-Casting with AABB intersection
// Useful for more complex geometry or irregular shapes
inline bool ray_aabb_intersection(const Ray &ray, const glm::vec3 &aabbMin,
                                   const glm::vec3 &aabbMax, float &tMin) {
  glm::vec3 invDir = 1.0f / ray.direction;
  glm::vec3 t0s = (aabbMin - ray.origin) * invDir;
  glm::vec3 t1s = (aabbMax - ray.origin) * invDir;

  glm::vec3 tsmaller = glm::min(t0s, t1s);
  glm::vec3 tbigger = glm::max(t0s, t1s);

  tMin = glm::max(tsmaller.x, glm::max(tsmaller.y, tsmaller.z));
  float tmax = glm::min(tbigger.x, glm::min(tbigger.y, tbigger.z));

  return tMin <= tmax && tmax >= 0.0f;
}

} // namespace CellPicking
