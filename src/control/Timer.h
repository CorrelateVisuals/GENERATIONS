// Frame/runtime time utility used by simulation and shader push constants.
// Exists to centralize time-scale and day-cycle progression semantics.
#include <cstdint>
#include <chrono>

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
  float hour_accumulator{0.0f};
  bool initialized{false};
  std::chrono::steady_clock::time_point last_update_time{};
};
