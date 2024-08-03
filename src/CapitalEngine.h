#pragma once
#include "Mechanics.h" // can't be forward declared

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
