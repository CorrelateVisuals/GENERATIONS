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
	  /// - Grid 
	  /// - Timer
	  /// - Behavior of cells
	  /// - Resources
	  /// - Mouse
	  /// - Camera
	  /// - Keyboard
	  };

  class World {
  public:
	  /// Textures
	  /// Geometry
	  ///	- Terrain
	  ///	- Shapes
  };

  class Engine {
  public:
	  /// Render
	  ///	- Pipelines
	  ///	- Shader traffic
	  ///		+ make and configure shader input, output and order.
	  ///			- with the purpose to take these modular shaders and rearrange and change them for various visual effects
  };

  class GUI {
  public:
	  /// Window
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
