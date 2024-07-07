#include "CapitalEngine.h"
#include "Interface.h"

#include <string>

int main() {
  Interface interface;
  // Log::text(interface.canvasses);

  // return EXIT_SUCCESS;

  try {
    CapitalEngine GENERATIONS;
    GENERATIONS.mainLoop();

  } catch (const std::exception& e) {
    Log::text(e.what());
    return EXIT_FAILURE;
  } catch (...) {
    throw std::runtime_error("\n!ERROR! Unknown error caught in main()");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
