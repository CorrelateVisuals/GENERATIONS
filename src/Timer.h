#include <cstdint>

class Timer {
 public:
  Timer(float initSpeed);
  ~Timer();

  float speed{1.0f};
  uint64_t passedHours{0};
  float dayFraction{0.0f};

  void run();

 private:
  const int HOURS_PER_DAY{24};
  const float TARGET_DURATION{1.0f / (speed / HOURS_PER_DAY)};
  const float TRIGGER_DELAY_UNDER_SPEED = 100.0f;
};
