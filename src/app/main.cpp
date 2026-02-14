#include "CapitalEngine.h"
#include "core/Log.h"
#include "implementation/ScriptChainerApp.h"

#include <cstdlib>
#include <string>

int main() {
  try {
    if (const char *script_graph = std::getenv("CE_SCRIPT_GRAPH")) {
      CE::Implementation::ScriptChainerApp::run(script_graph);
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
