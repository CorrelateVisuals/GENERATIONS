#pragma once

#include <type_traits>
#include <typeinfo>
#include <stdexcept>
#include <string>
#include <utility>

#include <vulkan/vulkan.h>

namespace CE {

uint32_t findMemoryType(const uint32_t typeFilter,
                        const VkMemoryPropertyFlags properties);

template <typename Checkresult, typename... Args>
void VULKAN_RESULT(Checkresult vkResult, Args &&...args) {
  using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
  std::string objectName = typeid(ObjectType).name();

  VkResult result = vkResult(std::forward<Args>(args)...);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName + "!");
  }
}

} // namespace CE
