#pragma once
// Namespaces
#include "Library.h"
#include "Log.h"

// Singletons
#include "Window.h"

// Engine modules
#include "BaseClasses.h"
#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"
#include "Interface.h"

// temp
#include "Timer.h"

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  VulkanMechanics mechanics;

  Control control;

  Resources resources;
  Pipelines pipelines;

  Engine engine;
  GUI gui;

  void drawFrame();
};
