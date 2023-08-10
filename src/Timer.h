#include <cstdint>

class Timer {
 public:
  Timer(){};
  ~Timer(){};

  struct Configuration {
    float speed = 10.0f;
    uint64_t passedHours{0};
    float dayFraction{0.0f};

    const int HOURS_PER_DAY{24};
    const float targetDuration{1.0f / (speed / HOURS_PER_DAY)};
  } config;

  void runTimer();
};
