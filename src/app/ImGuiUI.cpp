#include "ImGuiUI.h"
#include "platform/Window.h"
#include "render/Mechanics.h"
#include "render/Pipelines.h"
#include "core/Log.h"

namespace CE {

ImGuiUI::ImGuiUI(VulkanMechanics &mechanics, Pipelines &pipelines) {
  Log::text("{ GUI }", "Initializing Dear ImGui");
  
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  
  // Create descriptor pool for ImGui
  create_descriptor_pool(mechanics.main_device.logical_device);
  
  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(Window::get().window, true);
  
  // Setup Vulkan backend
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = mechanics.init_vulkan.instance;
  init_info.PhysicalDevice = mechanics.main_device.physical_device;
  init_info.Device = mechanics.main_device.logical_device;
  init_info.QueueFamily = mechanics.queues.indices.graphics_and_compute_family.value();
  init_info.Queue = mechanics.queues.graphics_queue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = imgui_descriptor_pool;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = static_cast<uint32_t>(mechanics.swapchain.images.size());
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = nullptr;

  ImGui_ImplVulkan_Init(&init_info, pipelines.render.render_pass);
  
  device = mechanics.main_device.logical_device;
  
  Log::text("{ GUI }", "Dear ImGui initialized");
}

ImGuiUI::~ImGuiUI() {
  Log::text("{ GUI }", "Shutting down Dear ImGui");
  cleanup_vulkan();
}

void ImGuiUI::create_descriptor_pool(VkDevice device) {
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_descriptor_pool) !=
      VK_SUCCESS) {
    throw std::runtime_error("\n!ERROR! Failed to create ImGui descriptor pool!");
  }

  Log::text("{ GUI }", "Created ImGui descriptor pool");
}

void ImGuiUI::upload_fonts(VkCommandPool command_pool, VkQueue queue) {
  if (!fonts_uploaded) {
    // Upload Fonts
    VkCommandBuffer command_buffer = CE::CommandBuffers::singular_command_buffer;
    CE::CommandBuffers::begin_singular_commands(command_pool, queue);
    ImGui_ImplVulkan_CreateFontsTexture();
    CE::CommandBuffers::end_singular_commands(command_pool, queue);
    ImGui_ImplVulkan_DestroyFontsTexture();
    fonts_uploaded = true;
    Log::text("{ GUI }", "ImGui fonts uploaded");
  }
}

void ImGuiUI::cleanup_vulkan() {
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (imgui_descriptor_pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);
      imgui_descriptor_pool = VK_NULL_HANDLE;
    }
  }
}

void ImGuiUI::new_frame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiUI::render(VkCommandBuffer command_buffer) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

void ImGuiUI::render_demo_window() {
  if (show_demo_window) {
    ImGui::ShowDemoWindow(&show_demo_window);
  }
  
  // Add a simple custom window
  ImGui::Begin("GENERATIONS Info");
  ImGui::Text("Dear ImGui integrated with CAPITAL Engine");
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Checkbox("Show Demo Window", &show_demo_window);
  ImGui::End();
}

} // namespace CE
