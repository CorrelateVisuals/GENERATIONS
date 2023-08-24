#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace ValidationLayers {
extern VkDebugUtilsMessengerEXT debugMessenger;
const std::vector<const char*> validation;

inline bool isValidationEnabled() {
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

void setupDebugMessenger(VkInstance instance);
void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo);
bool checkValidationLayerSupport();
void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);

namespace Internal {
void LogValidationMessage(const std::string& string,
                          const std::string& excludeError);
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData);
}  // namespace Internal
}  // namespace ValidationLayers
