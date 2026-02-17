#include "VulkanSync.h"
#include "VulkanUtils.h"

#include "../core/Log.h"

#include <algorithm>
#include <limits>

VkCommandBuffer CE::CommandBuffers::singular_command_buffer = VK_NULL_HANDLE;

CE::CommandBuffers::~CommandBuffers() {
  if (Device::base_device && this->pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(Device::base_device->logical_device, this->pool, nullptr);
  }
};

CE::SingleUseCommands::SingleUseCommands(const VkCommandPool &command_pool,
                                         const VkQueue &queue)
    : command_pool_(command_pool), queue_(queue) {
  CommandBuffers::begin_singular_commands(command_pool_, queue_);
}

CE::SingleUseCommands::~SingleUseCommands() {
  if (!submitted_ && Device::base_device &&
      CommandBuffers::singular_command_buffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(Device::base_device->logical_device,
                         command_pool_,
                         1,
                         &CommandBuffers::singular_command_buffer);
    CommandBuffers::singular_command_buffer = VK_NULL_HANDLE;
  }
}

VkCommandBuffer &CE::SingleUseCommands::command_buffer() {
  return CommandBuffers::singular_command_buffer;
}

void CE::SingleUseCommands::submit_and_wait() {
  if (!submitted_) {
    CommandBuffers::end_singular_commands(command_pool_, queue_);
    submitted_ = true;
  }
}

void CE::CommandBuffers::create_pool(const Queues::FamilyIndices &family_indices) {
  Log::text("{ cmd }", "Command Pool");
  if (!Device::base_device) {
    Log::text("{ cmd }", "Command Pool: base_device is null");
  } else {
    Log::text("{ cmd }",
              "Command Pool: device",
              Device::base_device->logical_device,
              "@",
              &Device::base_device->logical_device);
  }
  Log::text("{ cmd }",
            "Command Pool: queue family",
            family_indices.graphics_and_compute_family.value());

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                   VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  poolInfo.queueFamilyIndex = family_indices.graphics_and_compute_family.value();

  const VkResult result =
        vkCreateCommandPool(
          Device::base_device->logical_device, &poolInfo, nullptr, &this->pool);
  Log::text("{ cmd }", "Command Pool created", result, this->pool, "@", &this->pool);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("!ERROR! vkCreateCommandPool failed!");
  }
}

void CE::CommandBuffers::begin_singular_commands(const VkCommandPool &command_pool,
                                                 const VkQueue &queue) {
  if (!Device::base_device || Device::base_device->logical_device == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "\n!ERROR! beginSingularCommands called without valid device.");
  }
  if (command_pool == VK_NULL_HANDLE || queue == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "\n!ERROR! beginSingularCommands called with null pool or queue.");
  }

  Log::text("{ 1.. }", "Begin Single Time CommandResources");
  Log::text("{ 1.. }",
            "Single Time: device",
            Device::base_device->logical_device,
            "@",
            &Device::base_device->logical_device);
  Log::text("{ 1.. }", "Single Time: pool", command_pool, "queue", queue);

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  const VkResult allocResult = vkAllocateCommandBuffers(
        Device::base_device->logical_device, &allocInfo, &singular_command_buffer);
      Log::text(
        "{ 1.. }", "Single Time alloc result", allocResult, singular_command_buffer);
  if (allocResult != VK_SUCCESS || singular_command_buffer == VK_NULL_HANDLE) {
    singular_command_buffer = VK_NULL_HANDLE;
    throw std::runtime_error("!ERROR! vkAllocateCommandBuffers failed for single time submit!");
  }

  VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags =
                                         VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  const VkResult beginResult = vkBeginCommandBuffer(singular_command_buffer, &beginInfo);
  Log::text("{ 1.. }", "Single Time begin result", beginResult);
  if (beginResult != VK_SUCCESS) {
    vkFreeCommandBuffers(
        Device::base_device->logical_device, command_pool, 1, &singular_command_buffer);
    singular_command_buffer = VK_NULL_HANDLE;
    throw std::runtime_error("!ERROR! vkBeginCommandBuffer failed for single time submit!");
  }

  return;
}

void CE::CommandBuffers::end_singular_commands(const VkCommandPool &command_pool,
                                               const VkQueue &queue) {
  if (!Device::base_device || Device::base_device->logical_device == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "\n!ERROR! endSingularCommands called without valid device.");
  }
    if (command_pool == VK_NULL_HANDLE || queue == VK_NULL_HANDLE ||
      singular_command_buffer == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! endSingularCommands called with invalid state.");
  }

  Log::text("{ ..1 }", "End Single Time CommandResources");
  Log::text("{ ..1 }", "Single Time: pool", command_pool, "queue", queue);

  const VkResult endResult = vkEndCommandBuffer(singular_command_buffer);
  Log::text("{ ..1 }", "Single Time end result", endResult);
  if (endResult != VK_SUCCESS) {
    vkFreeCommandBuffers(
        Device::base_device->logical_device, command_pool, 1, &singular_command_buffer);
    singular_command_buffer = VK_NULL_HANDLE;
    throw std::runtime_error("!ERROR! vkEndCommandBuffer failed for single time submit!");
  }
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &singular_command_buffer;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence uploadFence = VK_NULL_HANDLE;
  const VkResult fenceCreateResult =
        vkCreateFence(
          Device::base_device->logical_device, &fenceInfo, nullptr, &uploadFence);
  Log::text("{ ..1 }",
            "Single Time fence create result",
            static_cast<int32_t>(fenceCreateResult));
  if (fenceCreateResult != VK_SUCCESS) {
    vkFreeCommandBuffers(
        Device::base_device->logical_device, command_pool, 1, &singular_command_buffer);
    singular_command_buffer = VK_NULL_HANDLE;
    throw std::runtime_error("!ERROR! vkCreateFence failed for single time submit!");
  }

  const VkResult submitResult = vkQueueSubmit(queue, 1, &submitInfo, uploadFence);
  Log::text("{ ..1 }", "Single Time submit result", submitResult);
  if (submitResult != VK_SUCCESS) {
    vkDestroyFence(Device::base_device->logical_device, uploadFence, nullptr);
    vkFreeCommandBuffers(
        Device::base_device->logical_device, command_pool, 1, &singular_command_buffer);
    singular_command_buffer = VK_NULL_HANDLE;
    throw std::runtime_error("!ERROR! vkQueueSubmit failed for single time submit!");
  }
  if (Log::gpu_trace_enabled()) {
    Log::text("{ QUE }",
              "Queue submit",
              "queue",
              queue,
              "cmd",
              singular_command_buffer,
              "fence",
              uploadFence);
  }

  const VkResult waitResult =
      vkWaitForFences(Device::base_device->logical_device,
              1,
              &uploadFence,
              VK_TRUE,
              UINT64_MAX);
  Log::text("{ ..1 }", "Single Time fence wait result", waitResult);
  if (waitResult != VK_SUCCESS) {
    vkDestroyFence(Device::base_device->logical_device, uploadFence, nullptr);
    vkFreeCommandBuffers(
        Device::base_device->logical_device, command_pool, 1, &singular_command_buffer);
    singular_command_buffer = VK_NULL_HANDLE;
    throw std::runtime_error("!ERROR! vkWaitForFences failed for single time submit!");
  }
  if (Log::gpu_trace_enabled()) {
    Log::text("{ LCK }", "Fence wait complete", uploadFence, "result", waitResult);
  }

  vkDestroyFence(Device::base_device->logical_device, uploadFence, nullptr);

  vkFreeCommandBuffers(
      Device::base_device->logical_device, command_pool, 1, &singular_command_buffer);
  Log::text("{ ..1 }", "Single Time freed", singular_command_buffer);
  singular_command_buffer = VK_NULL_HANDLE;
}

void CE::CommandBuffers::create_buffers(
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> &command_buffers) const {
  Log::text("{ cmd }", "Command Buffers:", MAX_FRAMES_IN_FLIGHT);
  if (!Device::base_device) {
    Log::text("{ cmd }", "Command Buffers: base_device is null");
  } else {
    Log::text("{ cmd }",
              "Command Buffers: device",
              Device::base_device->logical_device,
              "@",
              &Device::base_device->logical_device);
  }
  Log::text("{ cmd }", "Command Buffers: pool", this->pool, "@", &this->pool);
  Log::text("{ cmd }",
            "Command Buffers: array",
            command_buffers.data(),
            "count",
            static_cast<uint32_t>(command_buffers.size()));
  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = this->pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(command_buffers.size())};

  const VkResult result = vkAllocateCommandBuffers(
      CE::Device::base_device->logical_device, &allocateInfo, command_buffers.data());
  Log::text("{ cmd }", "Command Buffers alloc result", result);
  for (size_t i = 0; i < command_buffers.size(); ++i) {
    Log::text(
        "{ cmd }", "Command Buffer", static_cast<uint32_t>(i), command_buffers[i]);
  }
  if (result != VK_SUCCESS) {
    throw std::runtime_error("!ERROR! vkAllocateCommandBuffers failed!");
  }
  Log::text("{ cmd }",
            "Command Buffers allocated",
            static_cast<uint32_t>(command_buffers.size()));
}

CE::Swapchain::SupportDetails
CE::Swapchain::check_support(const VkPhysicalDevice &physical_device,
                             const VkSurfaceKHR &surface) {
  Log::text(Log::Style::char_leader, "Query Swap Chain Support");
  {
    Swapchain::SupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device, surface, &details.capabilities);
    uint32_t formatCount(0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
      details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount(0);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.present_modes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &presentModeCount, details.present_modes.data());
    }

    Log::text("{ SWP }",
              Log::function_name(__func__),
              "Swapchain support",
              "formats",
              details.formats.size(),
              "presentModes",
              details.present_modes.size());
    Log::text(Log::Style::char_leader,
              "capabilities min/max imageCount",
              details.capabilities.minImageCount,
              "/",
              details.capabilities.maxImageCount);

    this->support_details = details;
    return details;
  }
}

VkSurfaceFormatKHR CE::Swapchain::pick_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats) const {
  Log::text(Log::Style::char_leader, "Choose Swap Surface Format");

  for (const auto &available_format : available_formats) {
    if (available_format.format == VK_FORMAT_R8G8B8A8_SRGB &&
        available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_format;
    }
  }
  return available_formats[0];
}

VkPresentModeKHR CE::Swapchain::pick_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes) const {
  Log::text(Log::Style::char_leader, "Choose Swap Present Mode");
  for (const auto &available_present_mode : available_present_modes) {
    if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return available_present_mode;
    }
  }

  for (const auto &available_present_mode : available_present_modes) {
    if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      return available_present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D CE::Swapchain::pick_extent(GLFWwindow *window,
                                      const VkSurfaceCapabilitiesKHR &capabilities) const {
  Log::text(Log::Style::char_leader, "Choose Swap Extent");

  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width(0), height(0);
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width = std::clamp(actualExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
                                     capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

uint32_t
CE::Swapchain::get_image_count(const Swapchain::SupportDetails &swapchain_support) const {
  uint32_t imageCount = swapchain_support.capabilities.minImageCount + 1;

  if (imageCount > MAX_FRAMES_IN_FLIGHT) {
    imageCount = MAX_FRAMES_IN_FLIGHT;
  }

  if (swapchain_support.capabilities.maxImageCount > 0 &&
      imageCount > swapchain_support.capabilities.maxImageCount) {
    imageCount = swapchain_support.capabilities.maxImageCount;
  }

  if (imageCount == 0) {
    imageCount = 1;
  }

  return imageCount;
}

void CE::Swapchain::destroy() {
  if (Device::base_device) {
    Log::text("{ <-> }", "Destroy Swapchain");

    for (uint_fast8_t i = 0; i < this->framebuffers.size(); i++) {
        vkDestroyFramebuffer(
          Device::base_device->logical_device, this->framebuffers[i], nullptr);
    }
    for (uint_fast8_t i = 0; i < this->images.size(); i++) {
        vkDestroyImageView(
          Device::base_device->logical_device, this->images[i].view, nullptr);
    }
    vkDestroySwapchainKHR(Device::base_device->logical_device, this->swapchain, nullptr);
  }
}

void CE::Swapchain::recreate(const VkSurfaceKHR &surface,
                             const Queues &queues,
                             SynchronizationObjects &sync_objects) {
  int width(0), height(0);
  // When minimized, many window systems report a 0x0 framebuffer.
  // Recreating swapchain resources at 0 size is invalid, so wait until visible again.
  glfwGetFramebufferSize(Window::get().window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Window::get().window, &width, &height);
    glfwWaitEvents();
  }

  // Recreate touches swapchain images/views/framebuffers that may still be referenced
  // by queued work. Device-wide idle makes this transition safe and deterministic.
  vkDeviceWaitIdle(Device::base_device->logical_device);

  destroy();
  create(surface, queues);

  // Keep frame index in a known slot after swapchain reset.
  constexpr uint32_t reset = 1;
  sync_objects.current_frame = reset;
}

void CE::Swapchain::create(const VkSurfaceKHR &surface, const Queues &queues) {
  Log::text("{ <-> }", "Swap Chain");
  const Swapchain::SupportDetails swapchainSupport =
      check_support(Device::base_device->physical_device, surface);
  const VkSurfaceFormatKHR surfaceFormat = pick_surface_format(swapchainSupport.formats);
  const VkPresentModeKHR presentMode = pick_present_mode(swapchainSupport.present_modes);
  const VkExtent2D extent = pick_extent(Window::get().window, support_details.capabilities);

  uint32_t imageCount = get_image_count(swapchainSupport);
  Log::text("{ SWP }",
            Log::function_name(__func__),
            "Requested swapchain imageCount",
            imageCount);

  VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      .preTransform = swapchainSupport.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE};

    const std::vector<uint32_t> queue_family_indices{
      queues.indices.graphics_and_compute_family.value(),
      queues.indices.present_family.value()};

    if (queues.indices.graphics_and_compute_family !=
      queues.indices.present_family) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queue_family_indices.size());
    createInfo.pQueueFamilyIndices = queue_family_indices.data();
    Log::text("{ SWP }",
              Log::function_name(__func__),
              "Sharing mode",
              "CONCURRENT",
              "gcFamily",
              queue_family_indices[0],
              "presentFamily",
              queue_family_indices[1]);
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    Log::text("{ SWP }",
              Log::function_name(__func__),
              "Sharing mode",
              "EXCLUSIVE",
              "family",
              queue_family_indices[0]);
  }

  CE::vulkan_result(vkCreateSwapchainKHR,
                    Device::base_device->logical_device,
                    &createInfo,
                    nullptr,
                    &this->swapchain);

  vkGetSwapchainImagesKHR(
      Device::base_device->logical_device, this->swapchain, &imageCount, nullptr);

  if (imageCount > MAX_FRAMES_IN_FLIGHT) {
    Log::text("{ SWP }",
              Log::function_name(__func__),
              "Clamping runtime swapchain images to MAX_FRAMES_IN_FLIGHT",
              imageCount,
              "->",
              MAX_FRAMES_IN_FLIGHT);
    imageCount = MAX_FRAMES_IN_FLIGHT;
  }

  Log::text("{ SWP }",
            Log::function_name(__func__),
            "Swapchain created",
            "format",
            static_cast<uint32_t>(surfaceFormat.format),
            "presentMode",
            static_cast<uint32_t>(presentMode),
            "extent",
            extent.width,
            "x",
            extent.height,
            "images",
            imageCount);

  this->image_format = surfaceFormat.format;
  this->extent = extent;

  std::vector<VkImage> swapchainImages(imageCount);
  vkGetSwapchainImagesKHR(
      Device::base_device->logical_device,
      this->swapchain,
      &imageCount,
      swapchainImages.data());

  for (uint_fast8_t i = 0; i < imageCount; i++) {
    this->images[i].image = swapchainImages[i];
    this->images[i].info.format = this->image_format;
    this->images[i].create_view(VK_IMAGE_ASPECT_COLOR_BIT);
  };
}

void CE::SynchronizationObjects::create() {
  Log::text("{ ||| }", "Sync Objects");

  VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CE::vulkan_result(vkCreateSemaphore,
              Device::base_device->logical_device,
              &semaphoreInfo,
              nullptr,
              &this->image_available_semaphores[i]);
    CE::vulkan_result(vkCreateSemaphore,
              Device::base_device->logical_device,
              &semaphoreInfo,
              nullptr,
              &this->render_finished_semaphores[i]);
    CE::vulkan_result(vkCreateFence,
              Device::base_device->logical_device,
              &fenceInfo,
              nullptr,
              &this->graphics_in_flight_fences[i]);
    CE::vulkan_result(vkCreateSemaphore,
              Device::base_device->logical_device,
              &semaphoreInfo,
              nullptr,
              &this->compute_finished_semaphores[i]);
    CE::vulkan_result(vkCreateFence,
              Device::base_device->logical_device,
              &fenceInfo,
              nullptr,
              &this->compute_in_flight_fences[i]);

    Log::text(Log::Style::char_leader,
              "frame",
              i,
              "sync handles",
              this->image_available_semaphores[i],
              this->compute_finished_semaphores[i],
              this->render_finished_semaphores[i],
              this->compute_in_flight_fences[i],
              this->graphics_in_flight_fences[i]);
  }
}

void CE::SynchronizationObjects::destroy() {
  if (!Device::base_device || Device::base_device->logical_device == VK_NULL_HANDLE) {
    return;
  }

  Log::text("{ ||| }", "Destroy Synchronization Objects");
  // Semaphores/fences can still be in use by in-flight submissions at shutdown.
  // Waiting for idle prevents VUID errors during vkDestroySemaphore/vkDestroyFence.
  vkDeviceWaitIdle(Device::base_device->logical_device);

  for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    // Null-handle guards make destruction idempotent and safe across partial init paths.
    if (this->render_finished_semaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(
          Device::base_device->logical_device,
          this->render_finished_semaphores[i],
          nullptr);
      this->render_finished_semaphores[i] = VK_NULL_HANDLE;
    }
    if (this->image_available_semaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(
          Device::base_device->logical_device,
          this->image_available_semaphores[i],
          nullptr);
      this->image_available_semaphores[i] = VK_NULL_HANDLE;
    }
    if (this->compute_finished_semaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(
          Device::base_device->logical_device,
          this->compute_finished_semaphores[i],
          nullptr);
      this->compute_finished_semaphores[i] = VK_NULL_HANDLE;
    }
    if (this->graphics_in_flight_fences[i] != VK_NULL_HANDLE) {
      vkDestroyFence(
          Device::base_device->logical_device,
          this->graphics_in_flight_fences[i],
          nullptr);
      this->graphics_in_flight_fences[i] = VK_NULL_HANDLE;
    }
    if (this->compute_in_flight_fences[i] != VK_NULL_HANDLE) {
      vkDestroyFence(
          Device::base_device->logical_device,
          this->compute_in_flight_fences[i],
          nullptr);
      this->compute_in_flight_fences[i] = VK_NULL_HANDLE;
    }
  }
}
