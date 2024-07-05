#pragma once
#include <array>
#include <string>
#include "glm/glm.hpp"

using Res = glm::ivec2;
using Chan = glm::ivec4;  // r g b a

struct Settings {
    Settings(Res r, Chan c = Chan(1, 1, 1, 1)) : resolution(r), channels(c) {}
    Res resolution;
    Chan channels;
};

struct Blend {
    int order{ 0 };
    int composite{ 0 };
};

struct Comp {
    Comp(Settings s, Blend b) : settings(s), blend(b) {}
    Settings settings;
    Blend blend;
};

class Interface {
public:
    Interface()
        : display(Res(1920, 1080)),  // Initialize display with specific values
        canvasses{
            {Comp(Settings(Res(1920, 1080), Chan(1, 1, 1, 1)), Blend{0, 1}),
             Comp(Settings(Res(1920, 1080), Chan(2, 2, 2, 2)), Blend{0, 1}),
             Comp(Settings(Res(1920, 1080), Chan(3, 3, 3, 3)), Blend{0, 1}),
             Comp(Settings(Res(1920, 1080), Chan(4, 4, 4, 4)), Blend{0, 1}),
             Comp(Settings(Res(1920, 1080), Chan(5, 5, 5, 5)), Blend{0, 1})} } {
        run();
    }
    ~Interface() {}
    Settings display;
    std::array<Comp, 5> canvasses;

    int run() { return 0; }  // Placeholder return value

private:
    int draw(std::string shape) { return 0; }  // Placeholder return value
    int canvas(glm::ivec2 resolution) { return 0; }  // Placeholder return value
};

int main() {
    Interface interface;
    interface.run();
    return 0;
};