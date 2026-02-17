#include "engine/CapitalEngine.h"
#include "engine/Log.h"
#include "world/RuntimeConfig.h"
#include "world/SceneConfig.h"

int main() {
  try {
    const CE::Scene::SceneConfig scene_config = CE::Scene::SceneConfig::defaults();
    scene_config.apply_to_runtime();

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
