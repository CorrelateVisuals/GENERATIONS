# Screenshot Module

## Overview
The Screenshot module provides functionality to capture and save Vulkan rendered frames to PNG image files. It reuses existing CAPITAL Engine components to minimize code duplication and maintain consistency with the project's architecture.

## Components Reused from the Codebase

The Screenshot implementation leverages the following existing components:

1. **CE::Buffer** - For staging buffers to transfer GPU image data to CPU memory
2. **CE::CommandBuffers** - For executing Vulkan commands (beginSingularCommands/endSingularCommands)
3. **CE::Device** - For accessing the logical device required for memory operations
4. **Log** - For consistent logging output matching the project's style
5. **stb_image_write.h** - Added to the libraries directory, following the pattern of stb_image.h

## Architecture

### Files
- **Screenshot.h** - Header file with class declaration
- **Screenshot.cpp** - Implementation of screenshot capture functionality
- **libraries/stb_image_write.h** - Third-party library for PNG writing (similar to stb_image.h)

### Design Decisions
- Placed in `CE` namespace to match other engine components
- Uses static methods as no state needs to be maintained between captures
- Follows the same coding style as BaseClasses.h and Resources.h
- Implements proper Vulkan image layout transitions to avoid disrupting rendering

## Usage Example

```cpp
#include "Screenshot.h"

// Inside your rendering loop or after a frame is presented:
void CapitalEngine::captureScreenshot() {
    // Assuming you have access to swapchain and mechanics objects
    const VkImage& currentImage = swapchain.images[currentFrame].image;
    const VkExtent2D& extent = swapchain.extent;
    const VkFormat& format = swapchain.imageFormat;
    
    CE::Screenshot::capture(
        currentImage,              // Source image from swapchain
        extent,                    // Image dimensions
        format,                    // Image format
        commands.pool,             // Command pool for transfer commands
        queues.graphics,           // Graphics queue
        "screenshots/capture.png"  // Output filename
    );
}
```

## Integration Points

### In CapitalEngine.cpp
To add screenshot capability triggered by a key press:

```cpp
#include "Screenshot.h"

void CapitalEngine::mainLoop() {
    while (!glfwWindowShouldClose(Window::get().window)) {
        glfwPollEvents();
        
        // Check for screenshot key (F12 for example)
        if (glfwGetKey(Window::get().window, GLFW_KEY_F12) == GLFW_PRESS) {
            takeScreenshot();
        }
        
        drawFrame();
    }
}

void CapitalEngine::takeScreenshot() {
    // Wait for device to be idle to ensure frame is complete
    vkDeviceWaitIdle(mechanics.device.logical);
    
    uint32_t currentFrame = mechanics.syncObjects.currentFrame;
    CE::Screenshot::capture(
        mechanics.swapchain.images[currentFrame].image,
        mechanics.swapchain.extent,
        mechanics.swapchain.imageFormat,
        mechanics.commands.pool,
        mechanics.queues.graphics,
        "screenshot.png"
    );
}
```

## Technical Details

### Image Layout Transitions
The Screenshot module properly handles Vulkan image layout transitions:
1. Transitions image from `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
2. Copies image data to staging buffer
3. Transitions image back to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`

This ensures the rendering pipeline is not disrupted.

### Color Format Conversion
The implementation handles BGRA to RGBA conversion, as Vulkan swapchains typically use BGRA format while PNG files expect RGBA.

### Memory Management
Uses staging buffers with host-visible memory, following the same pattern as texture loading in `BaseClasses.cpp`.

## Dependencies
- Vulkan SDK
- stb_image_write.h (included in libraries/)
- Existing CAPITAL Engine components (Buffer, CommandBuffers, Device, Log)

## Build System
The CMakeLists.txt has been updated to include the `libraries/` directory in the include paths, allowing the Screenshot module to access stb_image_write.h.

## Future Enhancements
Possible improvements that could be added while maintaining minimal changes:
- Support for different image formats (JPEG, BMP, TGA)
- Automatic timestamped filenames
- Screenshot directory creation if it doesn't exist
- Compressed image support
