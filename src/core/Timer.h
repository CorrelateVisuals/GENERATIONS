#include <cstdint>

class Timer {
public:
  Timer(float init_speed);
  virtual ~Timer() = default;

  uint64_t passed_hours{0};

  void run();
  float get_day_fraction() const;

private:
  float speed{1.0f};
  float day_fraction{0.0f};
  static constexpr int hours_per_day{24};
  const float target_duration{1.0f / (speed / hours_per_day)};
  static constexpr float trigger_delay_under_speed = 100.0f;
};
