#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class ValidationLayers {
public:
  ValidationLayers();
  ValidationLayers(const ValidationLayers &) = delete;
  ValidationLayers &operator=(const ValidationLayers &) = delete;
  ValidationLayers(ValidationLayers &&) = delete;
  ValidationLayers &operator=(ValidationLayers &&) = delete;
  ~ValidationLayers();

  VkDebugUtilsMessengerEXT debug_messenger;
  const std::vector<const char *> validation;

#ifdef NDEBUG
  const bool enable_validation_layers = false;
#else
  const bool enable_validation_layers = true;
#endif

  void setup_debug_messenger(VkInstance instance);
  void populate_debug_messenger_create_info(
      VkDebugUtilsMessengerCreateInfoEXT &create_info);
  bool check_validation_layer_support();
  void destroy_debug_utils_messenger_ext(VkInstance instance,
                                         VkDebugUtilsMessengerEXT debug_messenger,
                                         const VkAllocationCallbacks *allocator);
  std::vector<std::string> get_available_layer_names() const;
  bool are_all_layers_available(const std::vector<std::string> &available) const;

private:
  static void log_validation_message(const std::string &message,
                                     const std::string &exclude_error);
  VkResult
  create_debug_utils_messenger_ext(VkInstance instance,
                                   const VkDebugUtilsMessengerCreateInfoEXT *create_info,
                                   const VkAllocationCallbacks *allocator,
                                   VkDebugUtilsMessengerEXT *debug_messenger);

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                 const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                 void *user_data) {
    const std::string debug_message = callback_data->pMessage;
    log_validation_message(debug_message, "Epic Games");
    return VK_FALSE;
  }
};
