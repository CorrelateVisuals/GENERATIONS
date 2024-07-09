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

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

  class Control {
  public:

	  };

  class Engine {
  public:

  };

  class GUI {
  public:
  };

  Control control;
  Engine engine;
  GUI gui;

 private:
  VulkanMechanics mechanics;
  Resources resources;
  Pipelines pipelines;

  void drawFrame();
};
