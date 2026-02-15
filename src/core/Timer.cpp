#include "Timer.h"

#include <chrono>

Timer::Timer(float init_speed) : speed{init_speed} {}

float Timer::get_day_fraction() const {
  return day_fraction;
}

void Timer::run() {
  const auto now = std::chrono::steady_clock::now();
  if (!initialized) {
    initialized = true;
    last_update_time = now;
    return;
  }

  const float delta_seconds = std::chrono::duration<float>(now - last_update_time).count();
  last_update_time = now;

  if (speed <= 0.0f) {
    day_fraction = 0.0f;
    return;
  }

  hour_accumulator += delta_seconds * speed;

  if (hour_accumulator >= 1.0f) {
    const uint64_t advanced_hours = static_cast<uint64_t>(hour_accumulator);
    passed_hours += advanced_hours;
    hour_accumulator -= static_cast<float>(advanced_hours);
  }

  const float total_hours = static_cast<float>(passed_hours % hours_per_day) + hour_accumulator;
  day_fraction = total_hours / static_cast<float>(hours_per_day);
}
