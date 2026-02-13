#pragma once
#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

#define STRINGIFICATION(x) #x

namespace Log {
enum LogLevel { LOG_OFF = 0, LOG_MINIMIAL = 1, LOG_MODERATE = 2, LOG_DETAILED = 3 };
static uint8_t logLevel = LOG_DETAILED;
extern std::ofstream logFile;
extern std::string previousTime;
extern std::string previousLine;
extern uint32_t repeatedLineCount;

struct Style {
  static std::string charLeader;
  static std::string indentSize;
  static std::string headerGuard;
  static int columnCount;
  static int columnCountOffset;
};
void logTitle();
void logFooter();

template <class T, class... Ts> void text(const T &first, const Ts &...inputs);
bool skipLogging(uint8_t logLevel, const std::string &icon);
void emitLine(const std::string &line);
void flushRepeatedLine();
bool gpu_trace_enabled();
void measureElapsedTime();
std::string function_name(const char *functionName);

std::string getBufferUsageString(const VkBufferUsageFlags &usage);
std::string getMemoryPropertyString(const VkMemoryPropertyFlags &properties);
std::string getDescriptorTypeString(const VkDescriptorType &type);
std::string getShaderStageString(const VkShaderStageFlags &flags);
std::string getSampleCountString(const VkSampleCountFlags &sampleCount);
std::string getImageUsageString(const VkImageUsageFlags &usage);

std::string returnDateAndTime();

template <class T> std::string toString(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

}; // namespace Log

template <class T, class... Ts> void Log::text(const T &first, const Ts &...inputs) {
  const std::string firstAsString = Log::toString(first);
  if (Log::skipLogging(logLevel, firstAsString)) {
    return;
  }

  if constexpr (std::is_same_v<T, std::vector<int>>) {
    static int elementCount = 0;
    std::ostringstream line;
    line << Style::charLeader << ' ';
    for (const auto &element : first) {
      if (elementCount % Style::columnCount == 0 && elementCount != 0) {
        Log::emitLine(line.str());
        line.str("");
        line.clear();
        line << Style::charLeader << ' ';
        elementCount = 0;
      }
      line << element << ' ';
      elementCount++;
    }
    Log::emitLine(line.str());
  } else {
    std::ostringstream line;
    line << first;
    ((line << ' ' << inputs), ...);

    const std::string currentLine = line.str();
    if (currentLine == previousLine) {
      repeatedLineCount++;
      return;
    }

    Log::flushRepeatedLine();
    Log::emitLine(currentLine);
    previousLine = currentLine;
  }
}
