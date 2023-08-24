#pragma once
#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace Log {

struct Style {
  static std::string charLeader;
  static std::string indentSize;
  static int columnCount;
};

extern std::ofstream logFile;
extern std::string previousTime;

template <class T, class... Ts>
void console(const T& first, const Ts&... inputs);

std::string getBufferUsageString(VkBufferUsageFlags usage);
std::string returnDateAndTime();
};  // namespace Log

template <class T, class... Ts>
void Log::console(const T& first, const Ts&... inputs) {
  if (!logFile.is_open()) {
    std::cerr << "\n!ERROR! Could not open logFile for writing" << std::endl;
    return;
  }
  std::string currentTime = returnDateAndTime();
  int columnCountOffset = 4;

  if (currentTime != previousTime) {
    std::cout << ' ' << currentTime;
    logFile << ' ' << currentTime;
  } else {
    std::string padding(
        static_cast<size_t>(Style::columnCount) + columnCountOffset, ' ');
    std::cout << padding;
    logFile << padding;
  }
  if constexpr (std::is_same_v<T, std::vector<int>>) {
    static int elementCount = 0;
    std::cout << ' ' << Style::charLeader << ' ';
    logFile << ' ' << Style::charLeader << ' ';
    for (const auto& element : first) {
      if (elementCount % Style::columnCount == 0 && elementCount != 0) {
        std::string spaces(
            static_cast<size_t>(Style::columnCount) + columnCountOffset, ' ');

        std::cout << '\n' << ' ' << spaces << Style::charLeader << ' ';
        logFile << '\n' << ' ' << spaces << Style::charLeader << ' ';

        elementCount = 0;
      }
      std::cout << element << ' ';
      logFile << element << ' ';
      elementCount++;
    }
    std::cout << '\n';
    logFile << '\n';
  } else {
    std::cout << ' ' << first;
    logFile << ' ' << first;
    ((std::cout << ' ' << inputs, logFile << ' ' << inputs), ...);
    std::cout << '\n';
    logFile << '\n';
  }
  previousTime = currentTime;
}