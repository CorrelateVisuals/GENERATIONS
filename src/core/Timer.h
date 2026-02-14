#include <cstdint>

class Timer {
public:
  Timer(float init_speed);
  virtual ~Timer() = default;

  float speed{1.0f};
  uint64_t passed_hours{0};
  float day_fraction{0.0f};

  void run();

private:
  const int hours_per_day{24};
  const float target_duration{1.0f / (speed / hours_per_day)};
  const float trigger_delay_under_speed = 100.0f;
};
