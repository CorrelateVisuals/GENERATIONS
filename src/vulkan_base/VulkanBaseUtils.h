#pragma once

// Vulkan helper utilities (type-safe error wrappers and small support helpers).
// Exists to standardize error handling and reduce repetitive call-site checks.

#include <type_traits>
#include <typeinfo>
#include <stdexcept>
#include <string>
#include <utility>

#include <vulkan/vulkan.h>

namespace CE {

uint32_t find_memory_type(const uint32_t type_filter,
                          const VkMemoryPropertyFlags properties);

template <typename Checkresult, typename... Args>
void vulkan_result(Checkresult vk_result, Args &&...args) {
  using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
  std::string objectName = typeid(ObjectType).name();

  VkResult result = vk_result(std::forward<Args>(args)...);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName + "!");
  }
}

} // namespace CE
