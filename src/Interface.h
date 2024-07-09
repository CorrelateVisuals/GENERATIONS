#pragma once
#include <array>
#include <string>
#include "glm/glm.hpp"

#include "Library.h"
#include "Log.h"

enum class Content {
    SCENE_001,
    SCENE_002
};

struct Settings {
  Settings(uivec2_fast16_t r = uivec2_fast16_t(1000, 1000),
           ivec4 c = ivec4(1, 1, 1, 1))
      : resolution(r), channels(c) {}
  uivec2_fast16_t resolution;
  ivec4 channels;
};

struct Blend {
  int order{0};
  int composite{0};
};

struct Comp {
  Comp(Settings s, ivec4 c, vec4 b, Blend bl)
      : settings(s), channels(c), background(b), blend(bl) {
    Log::text("\n!!!!!!", s.channels.a, s.resolution.x);
  }
  Comp()
      : settings{uivec2_fast16_t(1000, 1000), ivec4(1, 1, 1, 1)},
        channels{1, 1, 1, 1},
        background{0.0f, 0.0f, 0.0f, 1.0f},
        blend{} {
    Log::text("Default constructor called\n");
  }
  Settings settings;
  ivec4 channels;
  vec4 background;
  Blend blend;
};

struct Display : Comp {
  Display(Settings s, ivec4 c, vec4 b, Blend bl) : Comp(s, c, b, bl) {}
};

struct Canvas : Comp {
  Canvas(Settings s, ivec4 c, vec4 b, Blend bl) : Comp(s, c, b, bl) {}
};

class Interface {
 public:
  Interface()
      : display(Settings(uivec2_fast16_t(1920, 1080), ivec4(1, 1, 1, 1)),
                ivec4(1, 1, 1, 1),
                vec4(0.0, 0.0, 0.0, 1.0),
                Blend{0, 1}),
        canvasses{Comp(Settings(uivec2_fast16_t(1920, 1080), ivec4(1, 1, 1, 1)),
                       ivec4(1, 1, 1, 1),
                       vec4(0.0, 0.0, 0.0, 1.0),
                       Blend{0, 1}),
                  Comp(Settings(uivec2_fast16_t(1920, 1080), ivec4(2, 2, 2, 2)),
                       ivec4(2, 2, 2, 2),
                       vec4(0.0, 0.0, 0.0, 1.0),
                       Blend{0, 1})} {
    run();
  }
  ~Interface() {}
  Display display;
  std::array<Comp, 2> canvasses;

  int run() { return 0; }

 private:
  int draw(std::string shape) { return 0; }
  int canvas(glm::ivec2 resolution) { return 0; }
};


