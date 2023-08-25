#include "Log.h"

#include <chrono>
#include <iostream>
#include <string>

namespace Log {
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

std::string getMemoryPropertyString(VkMemoryPropertyFlags properties) {
  std::string result;

  if (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
    result += "DEVICE_LOCAL | ";
  }
  if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
    result += "HOST_VISIBLE | ";
  }
  if (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
    result += "HOST_COHERENT | ";
  }
  if (properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
    result += "HOST_CACHED | ";
  }
  if (properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
    result += "LAZILY_ALLOCATED | ";
  }

  // Remove the trailing " | " if there is one.
  if (!result.empty()) {
    result.erase(result.length() - 3);
  }

  return result;
}

std::string Log::getDescriptorTypeString(VkDescriptorType type) {
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

std::string getShaderStageFlagString(VkShaderStageFlags flags) {
  std::string result;

  if (flags & VK_SHADER_STAGE_VERTEX_BIT) {
    result += "VK_SHADER_STAGE_VERTEX_BIT | ";
  }
  if (flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
    result += "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | ";
  }
  if (flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
    result += "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | ";
  }
  if (flags & VK_SHADER_STAGE_GEOMETRY_BIT) {
    result += "VK_SHADER_STAGE_GEOMETRY_BIT | ";
  }
  if (flags & VK_SHADER_STAGE_FRAGMENT_BIT) {
    result += "VK_SHADER_STAGE_FRAGMENT_BIT | ";
  }
  if (flags & VK_SHADER_STAGE_COMPUTE_BIT) {
    result += "VK_SHADER_STAGE_COMPUTE_BIT | ";
  }
  if (flags & VK_SHADER_STAGE_ALL_GRAPHICS) {
    result += "VK_SHADER_STAGE_ALL_GRAPHICS | ";
  }
  if (flags & VK_SHADER_STAGE_ALL) {
    result += "VK_SHADER_STAGE_ALL | ";
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
}  // namespace Log
