#pragma once

// Central logging and diagnostics utility.
// Exists to provide consistent runtime tracing for Vulkan, perf, and debug flows.
#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

// Two-level stringify: ensures macro arguments are fully expanded before stringification.
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

#define LOG_LEVEL_STRING(level) (level == Log::LOG_OFF ? "OFF" : \
                                 level == Log::LOG_MINIMAL ? "MINIMAL" : \
                                 level == Log::LOG_MODERATE ? "MODERATE" : \
                                 level == Log::LOG_DETAILED ? "DETAILED" : "UNKNOWN")

namespace Log {
enum LogLevel { LOG_OFF = 0, LOG_MINIMAL = 1, LOG_MODERATE = 2, LOG_DETAILED = 3 };
inline uint8_t log_level = LOG_MODERATE;
extern std::ofstream log_file;
extern std::string previous_time;
extern std::string previous_line;
extern uint32_t repeated_line_count;

struct Style {
  static std::string char_leader;
  static std::string indent_size;
  static std::string header_guard;
  static int column_count;
  static int column_count_offset;
};
void log_title();
void log_footer();

template <class T, class... Ts> void text(const T &first, const Ts &...inputs);
bool skip_logging(uint8_t log_level, const std::string &icon);
void emit_line(const std::string &line);
void flush_repeated_line();
bool gpu_trace_enabled();
void measure_elapsed_time();
std::string function_name(const char *function_name);

std::string get_buffer_usage_string(const VkBufferUsageFlags &usage);
std::string get_memory_property_string(const VkMemoryPropertyFlags &properties);
std::string get_descriptor_type_string(const VkDescriptorType &type);
std::string get_shader_stage_string(const VkShaderStageFlags &flags);
std::string get_sample_count_string(const VkSampleCountFlags &sample_count);
std::string get_image_usage_string(const VkImageUsageFlags &usage);

std::string return_date_and_time();

template <class T> std::string to_string(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
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
