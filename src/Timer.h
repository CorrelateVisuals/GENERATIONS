#include <cstdint>

class Control {
 public:
  struct Timer {
    float speed = 10.0f;
    uint64_t passedHours{0};
    float dayFraction{0.0f};

    const int HOURS_PER_DAY{24};
    const float targetDuration{1.0f / (speed / HOURS_PER_DAY)};
  } timer;

  void runTimer();
};
