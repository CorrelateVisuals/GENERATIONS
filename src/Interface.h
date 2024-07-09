#pragma once
#include <array>
#include <string>
#include "glm/glm.hpp"

#include "Timer.h"

class Control {
public:
	float speed = 25.0f;
	Timer time{ speed };
	/// - Grid
	/// - Timer
	/// - Mouse & keyboard
	/// - Camer/light controls
	/// - Assets (Resources)
	///	+ Textures
	///	+ Data
};

//class World {
//public:
	/// Geometries
	///	- Terrain (singular)
	///	- Shapes  (can be instanced)
	///	- Shapees ...
	/// Camera(s)
	/// Light(s)
//};

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