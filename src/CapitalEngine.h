#pragma once

// Debug namespaces
#include "Log.h"
#include "ValidationLayers.h"

// Singleton classes
#include "Window.h"

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

// class Global {
//  public:
//   Global() = default;
//
//   class Objects {
//    public:
//     Objects() = default;
//     ~Objects() = default;
//
//     // VulkanMechanics mechanics;
//     // Pipelines pipelines;
//     Resources memory;
//   };
//   inline static Objects obj;
//
//  private:
// };
//
//// inline static auto& _mechanics = Global::obj.mechanics;
//// inline static auto& _pipelines = Global::obj.pipelines;
// inline static auto& _resources = Global::obj.memory;
