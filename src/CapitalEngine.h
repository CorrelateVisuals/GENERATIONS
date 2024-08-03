#pragma once
#include "Log.h"

class VulkanMechanics;
class Resources;
class Pipelines;

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
