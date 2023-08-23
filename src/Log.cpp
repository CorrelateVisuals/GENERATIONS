#include "Log.h"

#include <chrono>
#include <iostream>
#include <string>

// Logging::Logging()
//     : logFile("log.txt", std::ofstream::out | std::ofstream::trunc) {
//   console("{ ... }", "constructing Logging");
// }
//
// Logging::~Logging() {
//   console("{ ... }", "destructing Logging");
// }

namespace Logging {
std::ofstream logFile = static_cast<std::ofstream>("log.txt");

std::string Style::charLeader = std::string(8, ' ') + ":";
std::string Style::indentSize = std::string(17, ' ');
int Style::columnCount = 14;

std::string previousTime;

std::string getBufferUsageString(VkBufferUsageFlags usage) {
  std::string result;

  if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
    result += "TRANSFER_SRC | ";
  }
  if (usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
    result += "TRANSFER_DST | ";
  }
  if (usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) {
    result += "UNIFORM_TEXEL_BUFFER | ";
  }
  if (usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) {
    result += "STORAGE_TEXEL_BUFFER | ";
  }
  if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
    result += "UNIFORM_BUFFER | ";
  }
  if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
    result += "STORAGE_BUFFER | ";
  }
  if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
    result += "INDEX_BUFFER | ";
  }
  if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
    result += "VERTEX_BUFFER | ";
  }
  if (usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
    result += "INDIRECT_BUFFER | ";
  }

  // Remove the trailing " | " if there is one.
  if (!result.empty()) {
    result.erase(result.length() - 3);
  }

  return result;
}

std::string returnDateAndTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t nowC = std::chrono::system_clock::to_time_t(now);
  std::tm timeInfo;

#ifdef __linux__
  char nowStr[20] = "---";
#elif _WIN32
  gmtime_s(&timeInfo, &nowC);
  char nowStr[20];
  strftime(nowStr, 20, "%y.%m.%d %H:%M:%S", &timeInfo);
#else

#endif

  return std::string(nowStr);
}
}  // namespace Logging
