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
	  /// - Mouse & keyboard
	  /// - Camer/light controls
	  /// - Assets (Resources)
	  ///	+ Textures
	  ///	+ Data
	  };

  class World {
  public:
	  /// Geometries
	  ///	- Terrain (singular)
	  ///	- Shapes  (can be instanced)
	  ///	- Shapees ...
	  /// Camera(s)
	  /// Light(s)
  };

  class Engine {
  public:
	  /// Render
	  ///	- Pipelines and Resources
	  ///	- Shaders
	  ///		+ descriptors
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
