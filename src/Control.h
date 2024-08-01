#pragma once
#include <array>
#include <string>
#include "glm/glm.hpp"

#include "Geometry.h"
#include "Library.h"
#include "Timer.h"

class Control {
 public:
  float speed{25.0f};
  Timer time{speed};

  struct Grid {
    uivec2_fast16_t size{100, 100};
    const uint_fast32_t initialAliveCells{5000};
  };

  /// - Mouse & keyboard
  /// - Camer/light controls
  /// - Assets (Resources)
  ///	+ Textures
  ///	+ Data
};

class Core {
 public:
  // Shader #1 rgba, 4*vec4 in in  4*vec4 out.
  // Shader #1 rgba, 4*vec4 in in  4*vec4 out.
  // Shader #1 rgba, 4*vec4 in in  4*vec4 out

  /// Render
  ///	- Pipelines and Resources
  ///	- Shaders
  ///		+ descriptors
  ///		+ make and configure shader input, output and order.
  ///			- with the purpose to take these modular shaders and
  /// rearrange and change them for various visual effects
};
