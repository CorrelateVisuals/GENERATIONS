#pragma once
#include <array>
#include <string>
#include "glm/glm.hpp"

#include "Library.h"
#include "Log.h"

using Chan = glm::ivec4;  // r g b a

struct Settings {
  Settings(ivec2_fast16_t r, Chan c = Chan(1, 1, 1, 1))
      : resolution(r), channels(c) {}
  ivec2_fast16_t resolution;
  Chan channels;
};

struct Blend {
  int order{0};
  int composite{0};
};

struct Comp {
  Comp(Settings s, Blend b) : settings(s), blend(b) {}
  Settings settings;
  Blend blend;
  // 2D 3D
};

class Interface {
 public:
  Interface()
      : display(ivec2_fast16_t(1920, 1080)),
        canvasses{{Comp(Settings(ivec2_fast16_t(1920, 1080), Chan(1, 1, 1, 1)),
                        Blend{0, 1}),
                   Comp(Settings(ivec2_fast16_t(1920, 1080), Chan(2, 2, 2, 2)),
                        Blend{0, 1})}} {
    run();
  }
  ~Interface() {}
  Settings display;
  std::array<Comp, 2> canvasses;

  int run() { return 0; }

 private:
  int draw(std::string shape) { return 0; }
  int canvas(glm::ivec2 resolution) { return 0; };
};
