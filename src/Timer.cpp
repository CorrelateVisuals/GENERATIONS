#include "Timer.h"

#include <chrono>
#include <iostream>
#include <thread>

Timer::Timer(){}

Timer::~Timer(){}

void Timer::run() {
    static auto lastTime = std::chrono::steady_clock::now();
    static std::chrono::time_point<std::chrono::steady_clock> dayStart =
        std::chrono::steady_clock::now();

    auto currentTime = std::chrono::steady_clock::now();

    if (currentTime - lastTime >=
        std::chrono::duration<float>(1.0f / speed)) {
        passedHours++;
        lastTime = currentTime;
    }

    std::chrono::duration<float> elapsedTime = currentTime - dayStart;
    std::chrono::duration<float> remainingTime =
        std::chrono::duration<float>(TARGET_DURATION) - elapsedTime;
    float elapsedSeconds = elapsedTime.count();
    float remainingSeconds = remainingTime.count();

    dayFraction = 1.0f - remainingSeconds / TARGET_DURATION;

    if (elapsedTime >= std::chrono::duration<float>(TARGET_DURATION)) {
        dayStart = currentTime;
    }

    if (speed <= TRIGGER_DELAY_UNDER_SPEED){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return;
}
