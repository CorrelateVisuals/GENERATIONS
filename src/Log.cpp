#include "Log.h"

#include <chrono>
#include <iostream>
#include <string>

namespace Log {
  
std::ofstream logFile("log.txt");

std::string Style::charLeader = std::string(8, ' ') + ": ";
std::string Style::indentSize = std::string(17, ' ');
std::string Style::headerGuard = std::string(
    "+-------------------------------------------------------------------------"
    "----+");
int Style::columnCount = 14;
int Style::columnCountOffset = 4;

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
  std::string result = "VkMemoryPropertyFlags: ";
#define ADD_FLAG_CASE(flag) \
  if (properties & flag) {  \
    result += #flag " | ";  \
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

std::string getDescriptorTypeString(VkDescriptorType type) {
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

std::string getShaderStageString(VkShaderStageFlags flags) {
  std::string result = "VkShaderStageFlags: ";
#define ADD_FLAG_CASE(flag) \
  if (flags & flag) {       \
    result += #flag " | ";  \
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

std::string getSampleCountString(VkSampleCountFlags sampleCount) {
  std::string result = "VkSampleCountFlags: ";
#define ADD_FLAG_CASE(flag) \
  if (sampleCount & flag) { \
    result += #flag " | ";  \
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

std::string getImageUsageString(VkImageUsageFlags usage) {
  std::string result = "VkImageUsageFlags: ";
#define ADD_FLAG_CASE(flag) \
  if (usage & flag) {       \
    result += #flag " | ";  \
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

std::string getMemoryPropertyFlags(VkMemoryPropertyFlags memFlags) {
  std::string result = "VkMemoryPropertyFlags: ";
#define ADD_FLAG_CASE(flag) \
  if (memFlags & flag) {    \
    result += #flag " | ";  \
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
