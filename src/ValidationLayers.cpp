#include "ValidationLayers.h"
#include "CapitalEngine.h"  // Assuming this is where Log is defined

#include <set>

namespace ValidationLayers {
VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

// ValidationLayers::ValidationLayers()
//     : debugMessenger{}, validation{ "VK_LAYER_KHRONOS_validation" } {
//     Log::console("{ --- }", "constructing Validation Layers");
// }

// ValidationLayers::~ValidationLayers() {
//     Log::console("{ --- }", "destructing Validation Layers");
// }

VKAPI_ATTR VkBool32 VKAPI_CALL Internal::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  const std::string debugMessage = pCallbackData->pMessage;
  Log::console(debugMessage, "Epic Games");
  return VK_FALSE;
}

void ValidationLayers::Internal::LogValidationMessage(
    const std::string& string,
    const std::string& excludeError) {
  if (string.find(excludeError) != std::string::npos)
    return;

  Log::console("\n\n                     > > > Validation Layer: ", string,
               "\n");
}

VkResult ValidationLayers::Internal::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void ValidationLayers::destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

void ValidationLayers::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = ValidationLayers::Internal::debugCallback};
}

void ValidationLayers::setupDebugMessenger(VkInstance instance) {
  if (!isValidationEnabled())
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  if (ValidationLayers::Internal::CreateDebugUtilsMessengerEXT(
          instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    throw std::runtime_error("\n!ERROR! Failed to set up debug messenger!");
}

bool ValidationLayers::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  std::set<std::string> availableLayerNames;
  for (const auto& layer : availableLayers) {
    availableLayerNames.insert(layer.layerName);
  }

  for (const auto& layerName : validation) {
    if (availableLayerNames.find(layerName) == availableLayerNames.end()) {
      return false;
    }
  }
  return true;
}

}  // namespace ValidationLayers
