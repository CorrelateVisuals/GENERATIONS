#include "CapitalEngine.h"
#include "core/Log.h"
#include "core/RuntimeConfig.h"
#include "implementation/ScriptChainerApp.h"

#include <cstdlib>
#include <string>

namespace {

bool env_truthy(const char *value) {
  if (!value) {
    return false;
  }
  const std::string mode(value);
  return mode == "1" || mode == "true" || mode == "TRUE" || mode == "on" ||
         mode == "ON";
}

} // namespace

int main() {
  try {
    const std::string script_graph =
        std::getenv("CE_SCRIPT_GRAPH") ? std::getenv("CE_SCRIPT_GRAPH")
                                        : "scripts/all_shaders_graph.py";
    CE::Implementation::ScriptChainerApp::run(script_graph);

    if (env_truthy(std::getenv("CE_SCRIPT_ONLY"))) {
      return EXIT_SUCCESS;
    }

    CE::Runtime::TerrainSettings terrain_settings{};
    CE::Runtime::set_terrain_settings(terrain_settings);

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
