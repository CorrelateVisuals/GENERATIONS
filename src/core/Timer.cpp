#include "Timer.h"

#include <chrono>
#include <thread>

Timer::Timer(float init_speed) : speed{init_speed} {}

void Timer::run() {
  static auto last_time = std::chrono::steady_clock::now();
  static std::chrono::time_point<std::chrono::steady_clock> day_start =
      std::chrono::steady_clock::now();

  auto current_time = std::chrono::steady_clock::now();

  if (current_time - last_time >= std::chrono::duration<float>(1.0f / speed)) {
    passed_hours++;
    last_time = current_time;
  }

  std::chrono::duration<float> elapsed_time = current_time - day_start;
  std::chrono::duration<float> remaining_time =
      std::chrono::duration<float>(target_duration) - elapsed_time;
  float elapsed_seconds = elapsed_time.count();
  float remaining_seconds = remaining_time.count();

  day_fraction = 1.0f - remaining_seconds / target_duration;

  if (elapsed_time >= std::chrono::duration<float>(target_duration)) {
    day_start = current_time;
  }

  if (speed <= trigger_delay_under_speed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return;
}
