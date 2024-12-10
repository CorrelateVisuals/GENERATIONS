#pragma once
#include "Mechanics.h"  // can't be forward declared
#include "Resources.h"
#include "Pipelines.h"

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  VulkanMechanics mechanics;
  Resources resources;
  Pipelines pipelines;

  void drawFrame();
};
