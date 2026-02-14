#include "ValidationLayers.h"

#include "core/Log.h"

#include <set>

ValidationLayers::ValidationLayers()
  : debug_messenger{}, validation{"VK_LAYER_KHRONOS_validation"} {}

ValidationLayers::~ValidationLayers() {}

void ValidationLayers::log_validation_message(const std::string &message,
                                              const std::string &exclude_error) {
  if (message.find(exclude_error) != std::string::npos)
    return;

  Log::text("!!!!!!!", "Validation Layer: ", message, "\n");
}

VkResult ValidationLayers::create_debug_utils_messenger_ext(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *create_info,
    const VkAllocationCallbacks *allocator,
    VkDebugUtilsMessengerEXT *debug_messenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func == nullptr) {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
  return func(instance, create_info, allocator, debug_messenger);
}

void ValidationLayers::destroy_debug_utils_messenger_ext(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks *allocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debug_messenger, allocator);
  }
}

void ValidationLayers::populate_debug_messenger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT &create_info) {
  create_info = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = debug_callback};
}

void ValidationLayers::setup_debug_messenger(VkInstance instance) {
  if (!enable_validation_layers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT create_info;
  populate_debug_messenger_create_info(create_info);

  if (create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) !=
      VK_SUCCESS)
    throw std::runtime_error("\n!ERROR! Failed to set up debug messenger!");
}

bool ValidationLayers::check_validation_layer_support() {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  std::set<std::string> available_layer_names;
  for (const auto &layer : available_layers) {
    available_layer_names.insert(layer.layerName);
  }

  for (const auto &layer_name : validation) {
    if (available_layer_names.find(layer_name) == available_layer_names.end()) {
      return false;
    }
  }
  return true;
}
