#include "CapitalEngine.h"

#include <filesystem>
#include <string>

int main() {
  try {
    Log::text("{info } current working dir: " +
              std::filesystem::current_path().string());
    CapitalEngine GENERATIONS;
    GENERATIONS.mainLoop();
  } catch (const std::exception &e) {
    Log::text(e.what());
    return EXIT_FAILURE;
  } catch (...) {
    throw std::runtime_error("\n!ERROR! Unknown error caught in main()");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
