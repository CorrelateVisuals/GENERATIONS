#pragma once
#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  VulkanMechanics mechanics;
  Resources resources;
  Pipelines pipelines;

  uint32_t lastPresentedImageIndex{0};

  void drawFrame();
  void takeScreenshot();
};
