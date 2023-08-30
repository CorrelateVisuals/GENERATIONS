#pragma once
// Namespaces
#include "Library.h"
#include "Log.h"

// Singleton classes
#include "Window.h"

// Vulkan modules
#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"
#include "ValidationLayers.h"

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  void drawFrame();
  void cleanup();
};
