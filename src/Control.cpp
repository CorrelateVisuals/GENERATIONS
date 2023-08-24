#include "Control.h"
#include "CapitalEngine.h"

#include <chrono>
#include <numbers>
#include <random>
#include <unordered_set>

Control::Control() : time{} {
  time.speed = timelineSpeed;
  Log::console("{ CTR }", "constructing Control");
}

Control::~Control() {
  Log::console("{ CTR }", "destructing Control");
}

void Control::setPushConstants() {
  _resources.pushConstants.data = {time.passedHours};
}

std::vector<uint_fast32_t> Control::setCellsAliveRandomly(
    uint_fast32_t numberOfCells) {
  std::vector<uint_fast32_t> CellIDs;
  CellIDs.reserve(numberOfCells);

  std::random_device random;
  std::mt19937 generate(random());
  std::uniform_int_distribution<int> distribution(
      0, _control.grid.dimensions[0] * _control.grid.dimensions[1] - 1);

  while (CellIDs.size() < numberOfCells) {
    int CellID = distribution(generate);
    if (std::find(CellIDs.begin(), CellIDs.end(), CellID) == CellIDs.end()) {
      CellIDs.push_back(CellID);
    }
  }
  std::sort(CellIDs.begin(), CellIDs.end());
  return CellIDs;
}
