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
static uint8_t log_level = LOG_DETAILED;
static uint8_t &logLevel = log_level;
extern std::ofstream log_file;
extern std::ofstream &logFile;
extern std::string previous_time;
extern std::string &previousTime;
extern std::string previous_line;
extern std::string &previousLine;
extern uint32_t repeated_line_count;
extern uint32_t &repeatedLineCount;

struct Style {
  static std::string char_leader;
  static std::string &charLeader;
  static std::string indent_size;
  static std::string &indentSize;
  static std::string header_guard;
  static std::string &headerGuard;
  static int column_count;
  static int &columnCount;
  static int column_count_offset;
  static int &columnCountOffset;
};
void log_title();
inline void logTitle() {
  log_title();
}
void log_footer();
inline void logFooter() {
  log_footer();
}

template <class T, class... Ts> void text(const T &first, const Ts &...inputs);
bool skip_logging(uint8_t log_level, const std::string &icon);
inline bool skipLogging(uint8_t current_log_level, const std::string &icon) {
  return skip_logging(current_log_level, icon);
}
void emit_line(const std::string &line);
inline void emitLine(const std::string &line) {
  emit_line(line);
}
void flush_repeated_line();
inline void flushRepeatedLine() {
  flush_repeated_line();
}
bool gpu_trace_enabled();
void measure_elapsed_time();
inline void measureElapsedTime() {
  measure_elapsed_time();
}
std::string function_name(const char *function_name);

std::string get_buffer_usage_string(const VkBufferUsageFlags &usage);
inline std::string getBufferUsageString(const VkBufferUsageFlags &usage) {
  return get_buffer_usage_string(usage);
}
std::string get_memory_property_string(const VkMemoryPropertyFlags &properties);
inline std::string getMemoryPropertyString(const VkMemoryPropertyFlags &properties) {
  return get_memory_property_string(properties);
}
std::string get_descriptor_type_string(const VkDescriptorType &type);
inline std::string getDescriptorTypeString(const VkDescriptorType &type) {
  return get_descriptor_type_string(type);
}
std::string get_shader_stage_string(const VkShaderStageFlags &flags);
inline std::string getShaderStageString(const VkShaderStageFlags &flags) {
  return get_shader_stage_string(flags);
}
std::string get_sample_count_string(const VkSampleCountFlags &sample_count);
inline std::string getSampleCountString(const VkSampleCountFlags &sample_count) {
  return get_sample_count_string(sample_count);
}
std::string get_image_usage_string(const VkImageUsageFlags &usage);
inline std::string getImageUsageString(const VkImageUsageFlags &usage) {
  return get_image_usage_string(usage);
}

std::string return_date_and_time();
inline std::string returnDateAndTime() {
  return return_date_and_time();
}

template <class T> std::string to_string(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}
template <class T> std::string toString(const T &value) {
  return to_string(value);
}

}; // namespace Log

template <class T, class... Ts> void Log::text(const T &first, const Ts &...inputs) {
  const std::string first_as_string = Log::to_string(first);
  if (Log::skip_logging(log_level, first_as_string)) {
    return;
  }

  if constexpr (std::is_same_v<T, std::vector<int>>) {
    static int elementCount = 0;
    std::ostringstream line;
    line << Style::char_leader << ' ';
    for (const auto &element : first) {
      if (elementCount % Style::column_count == 0 && elementCount != 0) {
        Log::emit_line(line.str());
        line.str("");
        line.clear();
        line << Style::char_leader << ' ';
        elementCount = 0;
      }
      line << element << ' ';
      elementCount++;
    }
    Log::emit_line(line.str());
  } else {
    std::ostringstream line;
    line << first;
    ((line << ' ' << inputs), ...);

    const std::string currentLine = line.str();
    if (currentLine == previous_line) {
      repeated_line_count++;
      return;
    }

    Log::flush_repeated_line();
    Log::emit_line(currentLine);
    previous_line = currentLine;
  }
}
