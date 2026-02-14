#include "CapitalEngine.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "implementation/ScriptChainerApp.h"

int main() {
  try {
    CE::Implementation::ScriptChainerApp::run();

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
