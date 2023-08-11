#include <cstdint>

class Timer {
 public:
  Timer( float timerSpeed );
  ~Timer();

  float speed;
  uint64_t passedHours;
  float dayFraction;

  void run();

private:
    const int HOURS_PER_DAY{24};
    const float TARGET_DURATION{ 1.0f / (speed / HOURS_PER_DAY) };
};
