#include "CapitalEngine.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "implementation/ScriptChainerApp.h"

#include <cstdlib>

int main() {
  try {
    const std::string script_graph =
        std::getenv("CE_SCRIPT_GRAPH") ? std::getenv("CE_SCRIPT_GRAPH")
                                        : "scripts/all_shaders_graph.py";
    CE::Implementation::ScriptChainerApp::run(script_graph);

    if (CE::Runtime::env_flag_enabled("CE_SCRIPT_ONLY")) {
      return EXIT_SUCCESS;
    }

    CapitalEngine GENERATIONS;
    GENERATIONS.main_loop();

  } catch (const std::exception &e) {
    Log::text(e.what());
    return EXIT_FAILURE;
  } catch (...) {
    throw std::runtime_error("\n!ERROR! Unknown error caught in main()");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
