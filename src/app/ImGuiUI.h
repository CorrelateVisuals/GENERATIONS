#pragma once
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

class VulkanMechanics;
class Resources;
class Pipelines;

namespace CE {

class ImGuiUI {
public:
  ImGuiUI(VulkanMechanics &mechanics, Pipelines &pipelines);
  ~ImGuiUI();

  void new_frame();
  void render(VkCommandBuffer command_buffer);
  void render_demo_window();
  void upload_fonts(VkCommandPool command_pool, VkQueue queue);

private:
  void create_descriptor_pool(VkDevice device);
  void cleanup_vulkan();

  VkDescriptorPool imgui_descriptor_pool{VK_NULL_HANDLE};
  VkDevice device{VK_NULL_HANDLE};
  bool show_demo_window{true};
  bool fonts_uploaded{false};
};

} // namespace CE
