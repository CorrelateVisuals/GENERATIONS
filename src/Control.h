#pragma once
#include "Timer.h"

#include <array>
#include <string>
#include <vector>

constexpr int off = -1;

class Timer;

class Control {
 public:
  Control();
  ~Control();

  Timer time;
  float timelineSpeed = 100.0f;

  struct Grid {
    uint_fast32_t totalAliveCells = 50000;
    std::array<uint_fast16_t, 2> dimensions = {250, 250};
  } grid;

  struct DisplayConfiguration {
    const char* title{"G E N E R A T I O N S"};
    uint16_t width = 1920;
    uint16_t height = 1080;
  } display;

  struct Compute {
    const uint8_t localSizeX{32};
    const uint8_t localSizeY{32};
    const uint8_t localSizeZ{1};
  } compute;

 public:
  std::vector<uint_fast32_t> setCellsAliveRandomly(uint_fast32_t numberOfCells);
  void setPushConstants();
};
