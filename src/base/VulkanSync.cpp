#include "VulkanSync.h"
#include "VulkanUtils.h"

#include "../core/Log.h"

#include <algorithm>
#include <limits>

VkCommandBuffer CE::CommandBuffers::singularCommandBuffer = VK_NULL_HANDLE;

CE::CommandBuffers::~CommandBuffers() {
  if (Device::base_device && this->pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(Device::base_device->logical_device, this->pool, nullptr);
  }
};

void CE::CommandBuffers::createPool(const Queues::FamilyIndices &family_indices) {
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

  VkResult result =
        vkCreateCommandPool(
          Device::base_device->logical_device, &poolInfo, nullptr, &this->pool);
  Log::text("{ cmd }", "Command Pool created", result, this->pool, "@", &this->pool);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("!ERROR! vkCreateCommandPool failed!");
  }
}

void CE::CommandBuffers::beginSingularCommands(const VkCommandPool &commandPool,
                                               const VkQueue &queue) {
  if (!Device::base_device || Device::base_device->logical_device == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "\n!ERROR! beginSingularCommands called without valid device.");
  }
  if (commandPool == VK_NULL_HANDLE || queue == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "\n!ERROR! beginSingularCommands called with null pool or queue.");
  }

  Log::text("{ 1.. }", "Begin Single Time CommandResources");
  Log::text("{ 1.. }",
            "Single Time: device",
            Device::base_device->logical_device,
            "@",
            &Device::base_device->logical_device);
  Log::text("{ 1.. }", "Single Time: pool", commandPool, "queue", queue);

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};

  VkResult allocResult = vkAllocateCommandBuffers(
      Device::base_device->logical_device, &allocInfo, &singularCommandBuffer);
  Log::text("{ 1.. }", "Single Time alloc result", allocResult, singularCommandBuffer);

  VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags =
                                         VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  VkResult beginResult = vkBeginCommandBuffer(singularCommandBuffer, &beginInfo);
  Log::text("{ 1.. }", "Single Time begin result", beginResult);

  return;
}

void CE::CommandBuffers::endSingularCommands(const VkCommandPool &commandPool,
                                             const VkQueue &queue) {
  if (!Device::base_device || Device::base_device->logical_device == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "\n!ERROR! endSingularCommands called without valid device.");
  }
  if (commandPool == VK_NULL_HANDLE || queue == VK_NULL_HANDLE ||
      singularCommandBuffer == VK_NULL_HANDLE) {
    throw std::runtime_error("\n!ERROR! endSingularCommands called with invalid state.");
  }

  Log::text("{ ..1 }", "End Single Time CommandResources");
  Log::text("{ ..1 }", "Single Time: pool", commandPool, "queue", queue);

  VkResult endResult = vkEndCommandBuffer(singularCommandBuffer);
  Log::text("{ ..1 }", "Single Time end result", endResult);
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &singularCommandBuffer;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence uploadFence = VK_NULL_HANDLE;
  VkResult fenceCreateResult =
        vkCreateFence(
          Device::base_device->logical_device, &fenceInfo, nullptr, &uploadFence);
  Log::text("{ ..1 }",
            "Single Time fence create result",
            static_cast<int32_t>(fenceCreateResult));
  if (fenceCreateResult != VK_SUCCESS) {
    vkFreeCommandBuffers(
        Device::base_device->logical_device, commandPool, 1, &singularCommandBuffer);
    throw std::runtime_error("!ERROR! vkCreateFence failed for single time submit!");
  }

  VkResult submitResult = vkQueueSubmit(queue, 1, &submitInfo, uploadFence);
  Log::text("{ ..1 }", "Single Time submit result", submitResult);

  VkResult waitResult =
      vkWaitForFences(Device::base_device->logical_device,
              1,
              &uploadFence,
              VK_TRUE,
              UINT64_MAX);
  Log::text("{ ..1 }", "Single Time fence wait result", waitResult);

  vkDestroyFence(Device::base_device->logical_device, uploadFence, nullptr);

  vkFreeCommandBuffers(
      Device::base_device->logical_device, commandPool, 1, &singularCommandBuffer);
  Log::text("{ ..1 }", "Single Time freed", singularCommandBuffer);
}

void CE::CommandBuffers::createBuffers(
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> &commandBuffers) const {
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
            commandBuffers.data(),
            "count",
            static_cast<uint32_t>(commandBuffers.size()));
  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = this->pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())};

  VkResult result = vkAllocateCommandBuffers(
      CE::Device::base_device->logical_device, &allocateInfo, commandBuffers.data());
  Log::text("{ cmd }", "Command Buffers alloc result", result);
  for (size_t i = 0; i < commandBuffers.size(); ++i) {
    Log::text("{ cmd }", "Command Buffer", static_cast<uint32_t>(i), commandBuffers[i]);
  }
  if (result != VK_SUCCESS) {
    throw std::runtime_error("!ERROR! vkAllocateCommandBuffers failed!");
  }
  Log::text("{ cmd }",
            "Command Buffers allocated",
            static_cast<uint32_t>(commandBuffers.size()));
}

CE::Swapchain::SupportDetails
CE::Swapchain::checkSupport(const VkPhysicalDevice &physicalDevice,
                            const VkSurfaceKHR &surface) {
  Log::text(Log::Style::charLeader, "Query Swap Chain Support");
  {
    Swapchain::SupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice, surface, &details.capabilities);
    uint32_t formatCount(0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physicalDevice, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount(0);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    Log::text("{ SWP }",
              Log::function_name(__func__),
              "Swapchain support",
              "formats",
              details.formats.size(),
              "presentModes",
              details.presentModes.size());
    Log::text(Log::Style::charLeader,
              "capabilities min/max imageCount",
              details.capabilities.minImageCount,
              "/",
              details.capabilities.maxImageCount);

    this->supportDetails = details;
    return details;
  }
}

VkSurfaceFormatKHR CE::Swapchain::pickSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) const {
  Log::text(Log::Style::charLeader, "Choose Swap Surface Format");

  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR CE::Swapchain::pickPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) const {
  Log::text(Log::Style::charLeader, "Choose Swap Present Mode");
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D CE::Swapchain::pickExtent(GLFWwindow *window,
                                     const VkSurfaceCapabilitiesKHR &capabilities) const {
  Log::text(Log::Style::charLeader, "Choose Swap Extent");

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
CE::Swapchain::getImageCount(const Swapchain::SupportDetails &swapchainSupport) const {
  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

  if (imageCount > MAX_FRAMES_IN_FLIGHT) {
    imageCount = MAX_FRAMES_IN_FLIGHT;
  }

  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
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
                             SynchronizationObjects &syncObjects) {
  int width(0), height(0);
  glfwGetFramebufferSize(Window::get().window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(Window::get().window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(Device::base_device->logical_device);

  destroy();
  create(surface, queues);

  uint32_t reset = 1;
  syncObjects.currentFrame = reset;
}

void CE::Swapchain::create(const VkSurfaceKHR &surface, const Queues &queues) {
  Log::text("{ <-> }", "Swap Chain");
  Swapchain::SupportDetails swapchainSupport =
      checkSupport(Device::base_device->physical_device, surface);
  VkSurfaceFormatKHR surfaceFormat = pickSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode = pickPresentMode(swapchainSupport.presentModes);
  VkExtent2D extent = pickExtent(Window::get().window, supportDetails.capabilities);

  uint32_t imageCount = getImageCount(swapchainSupport);
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

    std::vector<uint32_t> queue_family_indices{
      queues.family_indices.graphics_and_compute_family.value(),
      queues.family_indices.present_family.value()};

  if (queues.family_indices.graphics_and_compute_family !=
      queues.family_indices.present_family) {
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

  CE::VULKAN_RESULT(vkCreateSwapchainKHR,
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

  this->imageFormat = surfaceFormat.format;
  this->extent = extent;

  std::vector<VkImage> swapchainImages(imageCount);
  vkGetSwapchainImagesKHR(
      Device::base_device->logical_device,
      this->swapchain,
      &imageCount,
      swapchainImages.data());

  for (uint_fast8_t i = 0; i < imageCount; i++) {
    this->images[i].image = swapchainImages[i];
    this->images[i].info.format = this->imageFormat;
    this->images[i].createView(VK_IMAGE_ASPECT_COLOR_BIT);
  };
}

void CE::SynchronizationObjects::create() {
  Log::text("{ ||| }", "Sync Objects");

  VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CE::VULKAN_RESULT(vkCreateSemaphore,
                      Device::base_device->logical_device,
                      &semaphoreInfo,
                      nullptr,
                      &this->imageAvailableSemaphores[i]);
    CE::VULKAN_RESULT(vkCreateSemaphore,
                      Device::base_device->logical_device,
                      &semaphoreInfo,
                      nullptr,
                      &this->renderFinishedSemaphores[i]);
    CE::VULKAN_RESULT(vkCreateFence,
                      Device::base_device->logical_device,
                      &fenceInfo,
                      nullptr,
                      &this->graphicsInFlightFences[i]);
    CE::VULKAN_RESULT(vkCreateSemaphore,
                      Device::base_device->logical_device,
                      &semaphoreInfo,
                      nullptr,
                      &this->computeFinishedSemaphores[i]);
    CE::VULKAN_RESULT(vkCreateFence,
                      Device::base_device->logical_device,
                      &fenceInfo,
                      nullptr,
                      &this->computeInFlightFences[i]);

    Log::text(Log::Style::charLeader,
              "frame",
              i,
              "sync handles",
              this->imageAvailableSemaphores[i],
              this->computeFinishedSemaphores[i],
              this->renderFinishedSemaphores[i],
              this->computeInFlightFences[i],
              this->graphicsInFlightFences[i]);
  }
}

void CE::SynchronizationObjects::destroy() const {
  if (Device::base_device) {
    Log::text("{ ||| }", "Destroy Synchronization Objects");
    for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(
          Device::base_device->logical_device,
          this->renderFinishedSemaphores[i],
          nullptr);
      vkDestroySemaphore(
          Device::base_device->logical_device,
          this->imageAvailableSemaphores[i],
          nullptr);
      vkDestroySemaphore(
          Device::base_device->logical_device,
          this->computeFinishedSemaphores[i],
          nullptr);
      vkDestroyFence(
          Device::base_device->logical_device,
          this->graphicsInFlightFences[i],
          nullptr);
      vkDestroyFence(
          Device::base_device->logical_device,
          this->computeInFlightFences[i],
          nullptr);
    };
  }
}
