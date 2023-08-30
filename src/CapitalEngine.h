#pragma once
// Namespaces
#include "Log.h"
#include "ValidationLayers.h"
#include "Library.h"

// Singleton classes
#include "Window.h"

// Vulkan modules
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
