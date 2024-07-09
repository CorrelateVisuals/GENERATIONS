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
	  /// Grid 
	  /// Timer
	  /// Behavior of cells
	  };

  class World {
  public:
	  /// Textures
	  /// Geometry
	  ///
  };

  class Engine {
  public:
	  /// Render
	  ///	- Pipelines
	  ///	- Shader traffic
  };

  class GUI {
  public:
	  /// Display
  };



 private:
  VulkanMechanics mechanics;
  Resources resources;
  Pipelines pipelines;

  Control control;
  Engine engine;
  GUI gui;

  void drawFrame();
};
