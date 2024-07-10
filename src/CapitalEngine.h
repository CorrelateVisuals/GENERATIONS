#pragma once
// Namespaces
#include "Library.h"
#include "Log.h"

// Singletons
#include "Window.h"

// Engine modules
#include "BaseClasses.h"
#include "Interface.h"
#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"

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

  Core core;
  GUI gui;

  void drawFrame();
};
