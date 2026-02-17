#include "Log.h"
#include "world/RuntimeConfig.h"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#ifdef __linux__
#include <unistd.h>
#endif

std::ofstream Log::log_file("log.txt");

std::string Log::Style::char_leader = std::string(8, ' ') + ": ";
std::string Log::Style::indent_size = std::string(17, ' ');
std::string Log::Style::header_guard = std::string(
    "+-------------------------------------------------------------------------"
    "----+");
int Log::Style::column_count = 14;
int Log::Style::column_count_offset = 4;

std::string Log::previous_time;
std::string Log::previous_line;
uint32_t Log::repeated_line_count = 0;

namespace {
constexpr const char *RESET = "\033[0m";
constexpr const char *DIM = "\033[2m";
constexpr const char *CYAN = "\033[36m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *RED = "\033[31m";
constexpr const char *MAGENTA = "\033[35m";

bool useColorOutput() {
  static const bool enabled = [] {
#ifdef __linux__
    if (std::getenv("NO_COLOR")) {
      return false;
    }
    const char *term = std::getenv("TERM");
    if (!term || std::string(term) == "dumb") {
      return false;
    }
    return isatty(fileno(stdout)) != 0;
#else
    return false;
#endif
  }();
  return enabled;
}

std::string extractIconToken(const std::string &line) {
  if (!line.empty() && line[0] == '{') {
    const size_t close = line.find('}');
    if (close != std::string::npos) {
      return line.substr(0, close + 1);
    }
  }
  return {};
}

const char *iconColor(const std::string &icon) {
  if (icon == "{ !!! }") {
    return RED;
  }
  if (icon == "{ PERF }" || icon == "{ TIME START }" || icon == "{ TIME INTERVAL }") {
    return MAGENTA;
  }
  if (icon == "{ >>> }" || icon == "{ GPU }" || icon == "{ SWP }") {
    return GREEN;
  }
  if (icon == "{ MEM }" || icon == "{ SYNC }") {
    return CYAN;
  }
  if (icon == "{ ... }" || icon == "{ 1.. }" || icon == "{ ..1 }") {
    return DIM;
  }
  return YELLOW;
}

std::string colorizeIcon(const std::string &line) {
  if (!useColorOutput()) {
    return line;
  }

  const std::string icon = extractIconToken(line);
  if (icon.empty()) {
    return line;
  }

  std::string colored = line;
  colored.replace(0,
                  icon.length(),
                  std::string(iconColor(icon)) + icon + RESET);
  return colored;
}

bool is_moderate_icon_suppressed(const std::string &icon) {
  if (icon.empty()) {
    return false;
  }

    static const std::array<std::string_view, 10> suppressed_icons = {
      "{ ... }",
      "{ 1.. }",
      "{ ..1 }",
      "{ MAP }",
      "{ WR }",
      "{ |=| }",
      "{ 101 }",
      "{ LCK }",
      "{ cmd }",
      "{ MEM }",
  };

  for (const std::string_view suppressed : suppressed_icons) {
    if (icon == suppressed) {
      return true;
    }
  }
  return false;
}

void log_ascii_banner() {
  static const std::array<std::string_view, 1> banner_lines = {
      "                 . - < < { G E N E R A T I O N S } > > - .",
  };

  for (const std::string_view line : banner_lines) {
    Log::text(line);
  }
}

void configure_log_level_once() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  const char *env = std::getenv("CE_LOG_LEVEL");
  if (!env) {
    return;
  }

  std::string value(env);
  for (char &c : value) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  if (value == "off" || value == "0") {
    Log::log_level = Log::LOG_OFF;
  } else if (value == "minimal" || value == "min" || value == "1") {
    Log::log_level = Log::LOG_MINIMIAL;
  } else if (value == "moderate" || value == "mod" || value == "2") {
    Log::log_level = Log::LOG_MODERATE;
  } else if (value == "detailed" || value == "detail" || value == "3") {
    Log::log_level = Log::LOG_DETAILED;
  }
}
} // namespace

std::string Log::function_name(const char *function_name) {
  if (!function_name) {
    return "_unknown_function";
  }

  std::string formatted{"_"};
  formatted.reserve(std::char_traits<char>::length(function_name) + 4);

  for (size_t i = 0; function_name[i] != '\0'; ++i) {
    const unsigned char current = static_cast<unsigned char>(function_name[i]);
    if (std::isupper(current)) {
      if (i > 0 && function_name[i - 1] != '_') {
        formatted.push_back('_');
      }
      formatted.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(current))));
    } else {
      formatted.push_back(static_cast<char>(current));
    }
  }

  return formatted;
}

void Log::measure_elapsed_time() {
  static std::chrono::high_resolution_clock::time_point last_call;
  static bool first_call = true;

  std::chrono::high_resolution_clock::time_point now =
      std::chrono::high_resolution_clock::now();

  if (first_call) {
    first_call = false;
    last_call = now;
    Log::text("{ TIME START }", "0.0", "seconds");
  } else {
    double elapsed_time =
        std::chrono::duration_cast<std::chrono::duration<double>>(now - last_call)
            .count();
    Log::text("{ TIME INTERVAL }", elapsed_time, "seconds");
    last_call = now;
  }
}

void Log::log_title() {
  Log::text(Log::Style::header_guard);
  log_ascii_banner();
  Log::text(Log::Style::header_guard);
  Log::measure_elapsed_time();

  Log::text("{ dir }", std::filesystem::current_path().string());
}

void Log::log_footer() {
  flush_repeated_line();
  Log::measure_elapsed_time();
  Log::text(Log::Style::header_guard);
  Log::text("                 << Jakob Povel | Correlate Visuals >>");
}

bool Log::skip_logging(uint8_t log_level, const std::string &icon) {
  configure_log_level_once();

  if (!log_file.is_open()) {
    std::cerr << "\n!ERROR! Could not open log_file for writing" << '\n';
    return false;
  }

  if (log_level == LOG_OFF ||
      (log_level == LOG_MINIMIAL &&
       (icon == std::string("{ ... }") || icon == std::string(Style::char_leader))) ||
      (log_level == LOG_MODERATE &&
       (icon == std::string(Style::char_leader) || is_moderate_icon_suppressed(icon)))) {
    return true;
  }
  return false;
}

bool Log::gpu_trace_enabled() {
  static const bool enabled = [] {
    return CE::Runtime::env_flag_enabled("CE_GPU_TRACE");
  }();

  return enabled;
}

void Log::emit_line(const std::string &line) {
  std::string current_time = return_date_and_time();
  if (current_time != previous_time) {
    std::cout << ' ' << current_time;
    log_file << ' ' << current_time;
  } else {
    std::string padding(
        static_cast<size_t>(Style::column_count) + Style::column_count_offset, ' ');
    std::cout << padding;
    log_file << padding;
  }

  std::cout << ' ' << colorizeIcon(line) << '\n';
  log_file << ' ' << line << '\n';
  previous_time = current_time;
}

void Log::flush_repeated_line() {
  if (repeated_line_count == 0) {
    return;
  }

  std::ostringstream summary;
  summary << "{ REP } previous line repeated" << ' ' << repeated_line_count << "x";
  emit_line(summary.str());
  repeated_line_count = 0;
}

std::string Log::get_buffer_usage_string(const VkBufferUsageFlags &usage) {
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

std::string Log::get_memory_property_string(const VkMemoryPropertyFlags &properties) {
  std::string result = "VkMemoryPropertyFlags: ";
#define ADD_FLAG_CASE(flag)                                                              \
  if (properties & flag) {                                                               \
    result += STRINGIFICATION(flag) " | ";                                               \
  }

  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_PROTECTED_BIT);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD);
  ADD_FLAG_CASE(VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV);

  if (!result.empty()) {
    result.erase(result.length() - 3);
  }

#undef ADD_FLAG_CASE
  return result;
}

std::string Log::get_descriptor_type_string(const VkDescriptorType &type) {
  switch (type) {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
      return "VK_DESCRIPTOR_TYPE_SAMPLER";
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
    case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      return "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK";
    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
      return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV";
    case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
      return "VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM";
    case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
      return "VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM";
    case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
      return "VK_DESCRIPTOR_TYPE_MUTABLE_EXT";
    default:
      return "Unknown VkDescriptorType";
  }
}

std::string Log::get_shader_stage_string(const VkShaderStageFlags &flags) {
  std::string result = "VkShaderStageFlags: ";
#define ADD_FLAG_CASE(flag)                                                              \
  if (flags & flag) {                                                                    \
    result += STRINGIFICATION(flag) " | ";                                               \
  }

  ADD_FLAG_CASE(VK_SHADER_STAGE_VERTEX_BIT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_GEOMETRY_BIT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_FRAGMENT_BIT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_COMPUTE_BIT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ADD_FLAG_CASE(VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
  ADD_FLAG_CASE(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  ADD_FLAG_CASE(VK_SHADER_STAGE_MISS_BIT_KHR);
  ADD_FLAG_CASE(VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
  ADD_FLAG_CASE(VK_SHADER_STAGE_CALLABLE_BIT_KHR);
  ADD_FLAG_CASE(VK_SHADER_STAGE_TASK_BIT_EXT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_MESH_BIT_EXT);
  ADD_FLAG_CASE(VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI);
  ADD_FLAG_CASE(VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI);
  ADD_FLAG_CASE(VK_SHADER_STAGE_RAYGEN_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_ANY_HIT_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_MISS_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_INTERSECTION_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_CALLABLE_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_TASK_BIT_NV);
  ADD_FLAG_CASE(VK_SHADER_STAGE_MESH_BIT_NV);

  if (!result.empty()) {
    result.erase(result.length() - 3);
  }

#undef ADD_FLAG_CASE

  return result;
}

std::string Log::get_sample_count_string(const VkSampleCountFlags &sample_count) {
  std::string result = "VkSampleCountFlags: ";
#define ADD_FLAG_CASE(flag)                                                              \
  if (sample_count & flag) {                                                             \
    result += STRINGIFICATION(flag) " | ";                                               \
  }

  ADD_FLAG_CASE(VK_SAMPLE_COUNT_1_BIT);
  ADD_FLAG_CASE(VK_SAMPLE_COUNT_2_BIT);
  ADD_FLAG_CASE(VK_SAMPLE_COUNT_4_BIT);
  ADD_FLAG_CASE(VK_SAMPLE_COUNT_8_BIT);
  ADD_FLAG_CASE(VK_SAMPLE_COUNT_16_BIT);
  ADD_FLAG_CASE(VK_SAMPLE_COUNT_32_BIT);
  ADD_FLAG_CASE(VK_SAMPLE_COUNT_64_BIT);

  if (!result.empty()) {
    result.erase(result.length() - 3);
  }

#undef ADD_FLAG_CASE

  return result;
}

std::string Log::get_image_usage_string(const VkImageUsageFlags &usage) {
  std::string result = "VkImageUsageFlags: ";
#define ADD_FLAG_CASE(flag)                                                              \
  if (usage & flag) {                                                                    \
    result += STRINGIFICATION(flag) " | ";                                               \
  }

  ADD_FLAG_CASE(VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_SAMPLED_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_STORAGE_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT);
  ADD_FLAG_CASE(VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR);

  if (!result.empty()) {
    result.erase(result.length() - 3);
  }

#undef ADD_FLAG_CASE
  return result;
}

std::string Log::return_date_and_time() {
  auto now = std::chrono::system_clock::now();
  std::time_t nowC = std::chrono::system_clock::to_time_t(now);
  char nowStr[20] = "---";

#ifdef __linux__
  std::tm timeInfo{};
  localtime_r(&nowC, &timeInfo);
  strftime(nowStr, sizeof(nowStr), "%y.%m.%d %H:%M:%S", &timeInfo);
#elif _WIN32
  std::tm timeInfo;
  gmtime_s(&timeInfo, &nowC);
  strftime(nowStr, sizeof(nowStr), "%y.%m.%d %H:%M:%S", &timeInfo);
#else

#endif

  return std::string(nowStr);
}
