#pragma once

// Debug objectless
#include "Log.h"
#include "ValidationLayers.h"

// Singleton
#include "Window.h"

#include "Control.h"
#include "Mechanics.h"
#include "Pipelines.h"
#include "Resources.h"
#include "World.h"

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  void drawFrame();
};

class Global {
 public:
  Global() = default;
  ~Global();

  class Objects {
   public:
    Objects() = default;
    ~Objects() = default;

    Control control;
    VulkanMechanics mechanics;
    Pipelines pipelines;
    Resources memory;
    // Window mainWindow;
    World world;
  };
  inline static Objects obj;

 private:
  void cleanup();
};

// inline static auto& Window::get() = Global::obj.mainWindow;
inline static auto& _mechanics = Global::obj.mechanics;
inline static auto& _pipelines = Global::obj.pipelines;
inline static auto& _resources = Global::obj.memory;
inline static auto& _control = Global::obj.control;
inline static auto& _world = Global::obj.world;
