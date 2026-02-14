# Practical Implementation Examples

This document provides practical code examples for implementing long-term maintainability features in the GENERATIONS codebase.

---

## 1. Runtime Vulkan Capability Detection

Detecting capabilities at runtime ensures graceful degradation and broad hardware support.

### Header: `src/base/VulkanCapabilities.h`

```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <set>

namespace CE::Base {

/**
 * @brief Runtime Vulkan capabilities detection
 * 
 * Detects available Vulkan features, extensions, and limits at runtime.
 * Used to enable/disable features based on hardware support.
 */
struct VulkanCapabilities {
    // API version
    uint32_t apiVersion = 0;
    uint32_t apiMajor = 0;
    uint32_t apiMinor = 0;
    uint32_t apiPatch = 0;
    
    // Device properties
    std::string deviceName;
    uint32_t driverVersion = 0;
    uint32_t vendorID = 0;
    uint32_t deviceID = 0;
    
    // Optional features
    bool geometryShader = false;
    bool tessellationShader = false;
    bool multiDrawIndirect = false;
    bool fillModeNonSolid = false;
    bool wideLines = false;
    bool samplerAnisotropy = false;
    
    // Compute capabilities
    uint32_t maxComputeWorkGroupInvocations = 0;
    uint32_t maxComputeWorkGroupSize[3] = {0, 0, 0};
    uint32_t maxComputeWorkGroupCount[3] = {0, 0, 0};
    
    // Memory limits
    uint64_t maxMemoryAllocationCount = 0;
    uint64_t bufferImageGranularity = 0;
    
    // Extensions
    std::set<std::string> availableExtensions;
    bool hasTimelineSemaphores = false;
    bool hasDynamicRendering = false;
    bool hasSwapchain = false;
    
    /**
     * @brief Detect capabilities from a physical device
     * 
     * @param physicalDevice The Vulkan physical device to query
     * @return Populated VulkanCapabilities struct
     */
    static VulkanCapabilities detect(VkPhysicalDevice physicalDevice);
    
    /**
     * @brief Print capabilities to console
     * 
     * Useful for debugging and system information display
     */
    void print() const;
    
    /**
     * @brief Check if a specific extension is available
     * 
     * @param extensionName The extension name to check (e.g., VK_KHR_timeline_semaphore)
     * @return true if extension is available
     */
    bool hasExtension(const char* extensionName) const;
};

} // namespace CE::Base
```

### Implementation: `src/base/VulkanCapabilities.cpp`

```cpp
#include "base/VulkanCapabilities.h"
#include "core/Logging.h"
#include <sstream>

namespace CE::Base {

VulkanCapabilities VulkanCapabilities::detect(VkPhysicalDevice physicalDevice) {
    VulkanCapabilities caps;
    
    // Get device properties
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    
    caps.apiVersion = props.apiVersion;
    caps.apiMajor = VK_VERSION_MAJOR(props.apiVersion);
    caps.apiMinor = VK_VERSION_MINOR(props.apiVersion);
    caps.apiPatch = VK_VERSION_PATCH(props.apiVersion);
    
    caps.deviceName = props.deviceName;
    caps.driverVersion = props.driverVersion;
    caps.vendorID = props.vendorID;
    caps.deviceID = props.deviceID;
    
    // Get device features
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    
    caps.geometryShader = features.geometryShader == VK_TRUE;
    caps.tessellationShader = features.tessellationShader == VK_TRUE;
    caps.multiDrawIndirect = features.multiDrawIndirect == VK_TRUE;
    caps.fillModeNonSolid = features.fillModeNonSolid == VK_TRUE;
    caps.wideLines = features.wideLines == VK_TRUE;
    caps.samplerAnisotropy = features.samplerAnisotropy == VK_TRUE;
    
    // Get compute limits
    caps.maxComputeWorkGroupInvocations = props.limits.maxComputeWorkGroupInvocations;
    caps.maxComputeWorkGroupSize[0] = props.limits.maxComputeWorkGroupSize[0];
    caps.maxComputeWorkGroupSize[1] = props.limits.maxComputeWorkGroupSize[1];
    caps.maxComputeWorkGroupSize[2] = props.limits.maxComputeWorkGroupSize[2];
    caps.maxComputeWorkGroupCount[0] = props.limits.maxComputeWorkGroupCount[0];
    caps.maxComputeWorkGroupCount[1] = props.limits.maxComputeWorkGroupCount[1];
    caps.maxComputeWorkGroupCount[2] = props.limits.maxComputeWorkGroupCount[2];
    
    // Get memory limits
    caps.maxMemoryAllocationCount = props.limits.maxMemoryAllocationCount;
    caps.bufferImageGranularity = props.limits.bufferImageGranularity;
    
    // Enumerate available extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    
    for (const auto& ext : extensions) {
        caps.availableExtensions.insert(ext.extensionName);
    }
    
    // Check for specific important extensions
    caps.hasSwapchain = caps.hasExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    caps.hasTimelineSemaphores = caps.hasExtension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    caps.hasDynamicRendering = caps.hasExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    
    return caps;
}

void VulkanCapabilities::print() const {
    std::ostringstream oss;
    
    oss << "\n=== Vulkan Capabilities ===\n";
    oss << "Device: " << deviceName << "\n";
    oss << "API Version: " << apiMajor << "." << apiMinor << "." << apiPatch << "\n";
    oss << "Vendor ID: 0x" << std::hex << vendorID << std::dec << "\n";
    oss << "Device ID: 0x" << std::hex << deviceID << std::dec << "\n";
    
    oss << "\n--- Features ---\n";
    oss << "Geometry Shader: " << (geometryShader ? "YES" : "NO") << "\n";
    oss << "Tessellation: " << (tessellationShader ? "YES" : "NO") << "\n";
    oss << "Multi-Draw Indirect: " << (multiDrawIndirect ? "YES" : "NO") << "\n";
    oss << "Sampler Anisotropy: " << (samplerAnisotropy ? "YES" : "NO") << "\n";
    
    oss << "\n--- Compute Limits ---\n";
    oss << "Max Work Group Invocations: " << maxComputeWorkGroupInvocations << "\n";
    oss << "Max Work Group Size: [" 
        << maxComputeWorkGroupSize[0] << ", "
        << maxComputeWorkGroupSize[1] << ", "
        << maxComputeWorkGroupSize[2] << "]\n";
    
    oss << "\n--- Extensions ---\n";
    oss << "Swapchain: " << (hasSwapchain ? "YES" : "NO") << "\n";
    oss << "Timeline Semaphores: " << (hasTimelineSemaphores ? "YES" : "NO") << "\n";
    oss << "Dynamic Rendering: " << (hasDynamicRendering ? "YES" : "NO") << "\n";
    oss << "Total Extensions: " << availableExtensions.size() << "\n";
    
    oss << "===========================\n";
    
    CE::log(oss.str());
}

bool VulkanCapabilities::hasExtension(const char* extensionName) const {
    return availableExtensions.find(extensionName) != availableExtensions.end();
}

} // namespace CE::Base
```

### Usage Example

```cpp
// In device initialization code
VkPhysicalDevice physicalDevice = selectPhysicalDevice();

// Detect capabilities
auto caps = CE::Base::VulkanCapabilities::detect(physicalDevice);
caps.print();  // Log to console

// Use capabilities to conditionally enable features
std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,  // Required
};

// Add optional extensions if available
if (caps.hasTimelineSemaphores) {
    deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    CE::log("Using timeline semaphores for improved synchronization");
} else {
    CE::log("Timeline semaphores not available, using binary semaphores");
}

if (caps.hasDynamicRendering) {
    deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    CE::log("Using dynamic rendering");
}

// Create device with detected extensions
VkDeviceCreateInfo deviceInfo = {};
deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
// ... rest of device creation
```

---

## 2. Graceful Degradation for Missing Features

Example of handling optional features with fallbacks:

### Synchronization with Optional Timeline Semaphores

```cpp
class SyncManager {
public:
    SyncManager(VkDevice device, const VulkanCapabilities& caps) 
        : device_(device), useTimelineSemaphores_(caps.hasTimelineSemaphores) 
    {
        if (useTimelineSemaphores_) {
            CE::log("Using timeline semaphores for frame synchronization");
            initTimelineSemaphores();
        } else {
            CE::log("Using binary semaphores + fences for frame synchronization");
            initBinarySemaphores();
        }
    }
    
    void waitForFrame(uint64_t frameNumber) {
        if (useTimelineSemaphores_) {
            // Modern path: wait on timeline semaphore
            VkSemaphoreWaitInfo waitInfo = {};
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &timelineSemaphore_;
            waitInfo.pValues = &frameNumber;
            
            vkWaitSemaphores(device_, &waitInfo, UINT64_MAX);
        } else {
            // Legacy path: wait on fence
            vkWaitForFences(device_, 1, &frameFences_[frameNumber % MAX_FRAMES], 
                           VK_TRUE, UINT64_MAX);
        }
    }
    
private:
    VkDevice device_;
    bool useTimelineSemaphores_;
    
    // Modern sync
    VkSemaphore timelineSemaphore_ = VK_NULL_HANDLE;
    
    // Legacy sync  
    static constexpr size_t MAX_FRAMES = 3;
    VkFence frameFences_[MAX_FRAMES] = {VK_NULL_HANDLE};
    
    void initTimelineSemaphores() {
        VkSemaphoreTypeCreateInfo typeInfo = {};
        typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        typeInfo.initialValue = 0;
        
        VkSemaphoreCreateInfo semInfo = {};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semInfo.pNext = &typeInfo;
        
        vkCreateSemaphore(device_, &semInfo, nullptr, &timelineSemaphore_);
    }
    
    void initBinarySemaphores() {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        for (size_t i = 0; i < MAX_FRAMES; i++) {
            vkCreateFence(device_, &fenceInfo, nullptr, &frameFences_[i]);
        }
    }
};
```

---

## 3. Dependency Version Tracking

### Automated Version Extraction Script

`tools/check_library_versions.py`:

```python
#!/usr/bin/env python3
"""
Check versions of vendored header-only libraries.
Run as part of quarterly maintenance.
"""

import re
import sys
from pathlib import Path
from typing import Optional

def extract_version(file_path: Path, patterns: list[str]) -> Optional[str]:
    """Try multiple regex patterns to extract version from file."""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            # Read first 20KB (versions usually in header)
            content = f.read(20000)
            
        for pattern in patterns:
            match = re.search(pattern, content, re.IGNORECASE | re.MULTILINE)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
    
    return None

def main():
    repo_root = Path(__file__).parent.parent
    libs_dir = repo_root / "libraries"
    
    libraries = {
        'vk_mem_alloc.h': [
            r'VMA_VERSION\s+(\d+\.\d+\.\d+)',
            r'version\s+(\d+\.\d+\.\d+)',
        ],
        'stb_image.h': [
            r'stb_image\s+-\s+v(\d+\.\d+)',
            r'version\s+(\d+\.\d+)',
        ],
        'stb_image_write.h': [
            r'stb_image_write\s+-\s+v(\d+\.\d+)',
            r'version\s+(\d+\.\d+)',
        ],
        'tiny_obj_loader.h': [
            r'version\s+(\d+\.\d+\.\d+(?:rc\d+)?)',
            r'TINYOBJLOADER_VERSION\s+"([^"]+)"',
        ],
    }
    
    print("Vendored Library Versions")
    print("=" * 50)
    
    all_found = True
    for lib_name, patterns in libraries.items():
        lib_path = libs_dir / lib_name
        
        if not lib_path.exists():
            print(f"{lib_name}: NOT FOUND")
            all_found = False
            continue
        
        version = extract_version(lib_path, patterns)
        
        if version:
            print(f"{lib_name}: {version}")
        else:
            print(f"{lib_name}: VERSION NOT DETECTED")
            all_found = False
    
    print("=" * 50)
    
    if not all_found:
        print("\n⚠️  Some versions could not be detected.")
        print("   Update libraries/VERSIONS.md manually.")
        return 1
    
    print("\n✅ All library versions detected successfully.")
    print("   Update libraries/VERSIONS.md with these versions.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
```

Make it executable:
```bash
chmod +x tools/check_library_versions.py
```

Run during quarterly maintenance:
```bash
./tools/check_library_versions.py
```

---

## 4. Platform-Specific Code Isolation

### Abstract Platform Interface

```cpp
// platform/IPlatform.h
#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace CE::Platform {

/**
 * @brief Platform abstraction interface
 * 
 * Isolates platform-specific code (windowing, input, etc.)
 * to enable easy porting to new platforms.
 */
class IPlatform {
public:
    virtual ~IPlatform() = default;
    
    // Window management
    virtual bool initWindow(int width, int height, const std::string& title) = 0;
    virtual void shutdownWindow() = 0;
    virtual bool shouldClose() const = 0;
    virtual void pollEvents() = 0;
    virtual void getFramebufferSize(int& width, int& height) const = 0;
    
    // Vulkan integration
    virtual VkSurfaceKHR createVulkanSurface(VkInstance instance) = 0;
    virtual std::vector<const char*> getRequiredVulkanExtensions() const = 0;
    
    // Input (if needed)
    virtual bool isKeyPressed(int keyCode) const = 0;
    virtual void getMousePosition(double& x, double& y) const = 0;
    
    // Platform info
    virtual std::string getPlatformName() const = 0;
    virtual std::string getPlatformVersion() const = 0;
};

/**
 * @brief Factory function to create platform instance
 * 
 * Automatically selects the appropriate platform implementation.
 */
std::unique_ptr<IPlatform> createPlatform();

} // namespace CE::Platform
```

### GLFW Implementation

```cpp
// platform/GLFWPlatform.cpp
#include "platform/IPlatform.h"
#include <GLFW/glfw3.h>

namespace CE::Platform {

class GLFWPlatform : public IPlatform {
public:
    bool initWindow(int width, int height, const std::string& title) override {
        if (!glfwInit()) {
            return false;
        }
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        
        window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        return window_ != nullptr;
    }
    
    void shutdownWindow() override {
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
    }
    
    bool shouldClose() const override {
        return glfwWindowShouldClose(window_);
    }
    
    void pollEvents() override {
        glfwPollEvents();
    }
    
    void getFramebufferSize(int& width, int& height) const override {
        glfwGetFramebufferSize(window_, &width, &height);
    }
    
    VkSurfaceKHR createVulkanSurface(VkInstance instance) override {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window_, nullptr, &surface) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }
        return surface;
    }
    
    std::vector<const char*> getRequiredVulkanExtensions() const override {
        uint32_t count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        return std::vector<const char*>(extensions, extensions + count);
    }
    
    bool isKeyPressed(int keyCode) const override {
        return glfwGetKey(window_, keyCode) == GLFW_PRESS;
    }
    
    void getMousePosition(double& x, double& y) const override {
        glfwGetCursorPos(window_, &x, &y);
    }
    
    std::string getPlatformName() const override {
        return "GLFW";
    }
    
    std::string getPlatformVersion() const override {
        return std::to_string(GLFW_VERSION_MAJOR) + "." +
               std::to_string(GLFW_VERSION_MINOR) + "." +
               std::to_string(GLFW_VERSION_REVISION);
    }
    
private:
    GLFWwindow* window_ = nullptr;
};

std::unique_ptr<IPlatform> createPlatform() {
    return std::make_unique<GLFWPlatform>();
}

} // namespace CE::Platform
```

This allows easily adding SDL2Platform, NativePlatform, etc. in the future without changing application code.

---

## 5. CMake Version Requirements

Add minimum version checks to CMakeLists.txt:

```cmake
# Require specific Vulkan version
find_package(Vulkan 1.2 REQUIRED)

if(Vulkan_FOUND)
    message(STATUS "Found Vulkan SDK version: ${Vulkan_VERSION}")
    
    # Warn if version is old
    if(Vulkan_VERSION VERSION_LESS "1.2.0")
        message(WARNING "Vulkan SDK ${Vulkan_VERSION} is older than recommended (1.2.0+)")
        message(WARNING "Some features may not be available")
    endif()
    
    # Error if too old
    if(Vulkan_VERSION VERSION_LESS "1.1.0")
        message(FATAL_ERROR "Vulkan SDK ${Vulkan_VERSION} is too old. Minimum required: 1.1.0")
    endif()
else()
    message(FATAL_ERROR "Vulkan SDK not found. Please install from https://vulkan.lunarg.com/")
endif()

# Require specific GLFW version
find_package(glfw3 3.3 REQUIRED)
if(glfw3_FOUND)
    message(STATUS "Found GLFW version: ${glfw3_VERSION}")
endif()

# Python for build scripts
find_package(Python3 3.6 REQUIRED COMPONENTS Interpreter)
message(STATUS "Found Python: ${Python3_VERSION}")
```

---

## Next Steps

1. **Implement VulkanCapabilities class** in `src/base/`
2. **Add capability detection** to device initialization
3. **Use capabilities** to enable/disable features
4. **Add version checks** to CMakeLists.txt
5. **Run version check script** to document current library versions
6. **Abstract platform layer** behind interface (future work)

---

**See Also**:
- [LONG_TERM_VIABILITY.md](LONG_TERM_VIABILITY.md) - Strategic overview
- [VULKAN_REQUIREMENTS.md](VULKAN_REQUIREMENTS.md) - Detailed Vulkan requirements
- [MAINTENANCE_SCHEDULE.md](MAINTENANCE_SCHEDULE.md) - When to apply these practices
