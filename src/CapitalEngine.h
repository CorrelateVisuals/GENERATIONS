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

#ifndef _DEBUG
#define _DEBUG 0
#endif

class CapitalEngine {
 public:
  CapitalEngine();
  ~CapitalEngine();

  void mainLoop();

 private:
  VulkanMechanics mechanics;
  Pipelines pipelines;
  Resources resources;

  void drawFrame();
};
