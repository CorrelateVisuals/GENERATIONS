#pragma once
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
#include <typeinfo>

namespace CE {

/**
 * @brief Check Vulkan result and throw exception on error
 * 
 * Template function for validating Vulkan API calls.
 * Throws a runtime_error if the result is not VK_SUCCESS.
 */
template <typename Checkresult, typename... Args>
void VULKAN_RESULT(Checkresult vkResult, Args&&... args) {
  using ObjectType = std::remove_pointer_t<std::decay_t<Checkresult>>;
  std::string objectName = typeid(ObjectType).name();

  VkResult result = vkResult(std::forward<Args>(args)...);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! result != VK_SUCCESS " + objectName +
                             "!");
  }
}

}  // namespace CE
