#include "Log.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

std::ofstream Log::logFile("log.txt");

std::string Log::Style::charLeader = std::string(8, ' ') + ": ";
std::string Log::Style::indentSize = std::string(17, ' ');
std::string Log::Style::headerGuard = std::string(
    "+-------------------------------------------------------------------------"
    "----+");
int Log::Style::columnCount = 14;
int Log::Style::columnCountOffset = 4;

std::string Log::previousTime;

void Log::measureElapsedTime() {
  static std::chrono::high_resolution_clock::time_point lastCall;
  static bool firstCall = true;

  std::chrono::high_resolution_clock::time_point now =
      std::chrono::high_resolution_clock::now();

  if (firstCall) {
    firstCall = false;
    lastCall = now;
    Log::text("{ TIME START }", "0.0", "seconds");
  } else {
    double elapsedTime =
        std::chrono::duration_cast<std::chrono::duration<double>>(now -
                                                                  lastCall)
            .count();
    Log::text("{ TIME INTERVAL }", elapsedTime, "seconds");
    lastCall = now;
  }
}

void Log::logTitle() {
  Log::text(Log::Style::headerGuard);
  Log::text("                 . - < < { ", "G E N E R A T I O N S",
            " } > > - .");
  Log::text(Log::Style::headerGuard);
  Log::measureElapsedTime();
#ifdef _DEBUG
  Log::text("{ DEBUG ON }", "compiling shaders");
#else
  Log::text("{ DEBUG OFF }", "not compiling shaders");
#endif
  Log::text("{ dir }", std::filesystem::current_path().string());
}

void Log::logFooter() {
  Log::measureElapsedTime();
  Log::text(Log::Style::headerGuard);
  Log::text("© Jakob Povel | Correlate Visuals ©");
}

bool Log::skipLogging(uint8_t logLevel, std::string icon) {
  if (!logFile.is_open()) {
    std::cerr << "\n!ERROR! Could not open logFile for writing" << std::endl;
    return false;
  }
  if (logLevel == LOG_OFF ||
      (logLevel == LOG_MINIMIAL && (icon == std::string("{ ... }") ||
                                    icon == std::string(Style::charLeader))) ||
      (logLevel == LOG_MODERATE && icon == std::string(Style::charLeader))) {
    return true;
  }
  return false;
}

std::string Log::getBufferUsageString(const VkBufferUsageFlags& usage) {
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

std::string Log::getMemoryPropertyString(
    const VkMemoryPropertyFlags& properties) {
  std::string result = "VkMemoryPropertyFlags: ";
#define ADD_FLAG_CASE(flag)                \
  if (properties & flag) {                 \
    result += STRINGIFICATION(flag) " | "; \
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

std::string Log::getDescriptorTypeString(const VkDescriptorType& type) {
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

std::string Log::getShaderStageString(const VkShaderStageFlags& flags) {
  std::string result = "VkShaderStageFlags: ";
#define ADD_FLAG_CASE(flag)                \
  if (flags & flag) {                      \
    result += STRINGIFICATION(flag) " | "; \
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

std::string Log::getSampleCountString(const VkSampleCountFlags& sampleCount) {
  std::string result = "VkSampleCountFlags: ";
#define ADD_FLAG_CASE(flag)                \
  if (sampleCount & flag) {                \
    result += STRINGIFICATION(flag) " | "; \
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

std::string Log::getImageUsageString(const VkImageUsageFlags& usage) {
  std::string result = "VkImageUsageFlags: ";
#define ADD_FLAG_CASE(flag)                \
  if (usage & flag) {                      \
    result += STRINGIFICATION(flag) " | "; \
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

std::string Log::returnDateAndTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t nowC = std::chrono::system_clock::to_time_t(now);

#ifdef __linux__
  char nowStr[20] = "---";
#elif _WIN32
  std::tm timeInfo;
  gmtime_s(&timeInfo, &nowC);
  char nowStr[20];
  strftime(nowStr, 20, "%y.%m.%d %H:%M:%S", &timeInfo);
#else

#endif

  return std::string(nowStr);
}
