#pragma once
#include "render/Mechanics.h"
#include "render/Pipelines.h"
#include "render/Resources.h"

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
