#include "Timer.h"

#include <chrono>
#include <iostream>
#include <thread>

void Control::runTimer() {
  auto lastTime = std::chrono::steady_clock::now();
  std::chrono::time_point<std::chrono::steady_clock> dayStart =
      std::chrono::steady_clock::now();

  while (true) {
    auto currentTime = std::chrono::steady_clock::now();

    if (currentTime - lastTime >=
        std::chrono::duration<float>(1.0f / timer.speed)) {
      timer.passedHours++;
      lastTime = currentTime;
      std::cout << "Passed hours: " << timer.passedHours << std::endl;
    }

    std::chrono::duration<float> elapsedTime = currentTime - dayStart;
    std::chrono::duration<float> remainingTime =
        std::chrono::duration<float>(timer.targetDuration) - elapsedTime;
    float elapsedSeconds = elapsedTime.count();
    float remainingSeconds = remainingTime.count();

    timer.dayFraction = 1.0f - remainingSeconds / timer.targetDuration;

    if (elapsedTime >= std::chrono::duration<float>(timer.targetDuration)) {
      dayStart = currentTime;
    }
    std::cout << "Day Fraction: " << timer.dayFraction << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
