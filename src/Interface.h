#pragma once
#include <array>
#include <string>
#include <tuple>
#include "glm/glm.hpp"

struct Resolution;
struct Blend;

using res = glm::ivec2;
using chan = glm::ivec4;
using comp = std::tuple<Resolution, Blend>;

struct Resolution {
  Resolution(res r, chan c) : resolution(r), channels(c){};
  res resolution;
  chan channels;
};

struct Blend {
  int order = 0;
  int composite = 1;
};

class Interface {
 public:
  Interface()
      : canvasses{
            {comp(Resolution(res(1920, 1080), chan(1, 1, 1, 1)), Blend(0, 1)),
             comp(Resolution(res(1920, 1080), chan(1, 1, 1, 1)), Blend(0, 1)),
             comp(Resolution(res(1920, 1080), chan(1, 1, 1, 1)), Blend(0, 1)),
             comp(Resolution(res(1920, 1080), chan(1, 1, 1, 1)), Blend(0, 1)),
             comp(Resolution(res(1920, 1080), chan(1, 1, 1, 1)),
                  Blend(0, 1))}} {
    run();
  };
  ~Interface() {}

  std::array<comp, 5> canvasses;

  int run(){};

 private:
  int draw(std::string shape){};
  int canvas(glm::ivec2 resolution){};
};
