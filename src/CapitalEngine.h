#pragma once

// Debug namespaces
#include "Log.h"
#include "ValidationLayers.h"

// Singleton classes
#include "Window.h"

#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  void drawFrame();
  void cleanup();
};
