#include "Timer.h"

#include <chrono>
#include <iostream>
#include <thread>

void Timer::runTimer() {
  auto lastTime = std::chrono::steady_clock::now();
  std::chrono::time_point<std::chrono::steady_clock> dayStart =
      std::chrono::steady_clock::now();

  while (true) {
    auto currentTime = std::chrono::steady_clock::now();

    if (currentTime - lastTime >=
        std::chrono::duration<float>(1.0f / config.speed)) {
      config.passedHours++;
      lastTime = currentTime;
      std::cout << "Passed hours: " << config.passedHours << std::endl;
    }

    std::chrono::duration<float> elapsedTime = currentTime - dayStart;
    std::chrono::duration<float> remainingTime =
        std::chrono::duration<float>(config.targetDuration) - elapsedTime;
    float elapsedSeconds = elapsedTime.count();
    float remainingSeconds = remainingTime.count();

    config.dayFraction = 1.0f - remainingSeconds / config.targetDuration;

    if (elapsedTime >= std::chrono::duration<float>(config.targetDuration)) {
      dayStart = currentTime;
    }
    std::cout << "Day Fraction: " << config.dayFraction << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
