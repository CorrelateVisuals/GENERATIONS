#pragma once
// Namespaces
#include "Library.h"
#include "Log.h"

// Singletons
#include "Window.h"

// Engine modules
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
  Pipelines pipelines;
  Resources resources;

  void drawFrame();
};
