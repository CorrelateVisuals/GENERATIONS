# GENERATIONS Architecture Documentation

## Overview

This document describes the architecture of the GENERATIONS engine, with a focus on the recent refactoring that improves baseclass segmentation and introduces a rendering interface abstraction layer.

## Rendering Architecture

### Abstraction Layers

The engine now uses a three-tiered architecture:

```
Application Code (CapitalEngine, Resources, Pipelines)
           ↓
   Rendering Interface (API-agnostic)
           ↓
   Vulkan Backend (Vulkan-specific implementation)
```

### 1. Rendering Interface Layer (`RenderInterface.h`)

The rendering interface provides API-agnostic abstractions for graphics operations. This layer allows rendering code to be written without direct knowledge of the underlying graphics API (Vulkan, OpenGL, DirectX, etc.).

**Key Interfaces:**

- **`IDevice`**: Device management and capabilities
  - Query device properties
  - Wait for device idle
  - Check feature support

- **`IBuffer`**: GPU buffer operations
  - Create/destroy buffers
  - Map/unmap memory
  - Copy operations

- **`IImage`**: Texture and image management
  - Load textures from files
  - Transition image layouts
  - Create image views

- **`IPipeline`**: Graphics and compute pipelines
  - Bind pipelines
  - Configure shader stages

- **`ICommandBuffer`**: GPU command recording
  - Begin/end command recording
  - Draw commands (draw, drawIndexed, dispatch)
  - Bind resources

- **`ISync`**: Synchronization primitives
  - Fences for CPU-GPU sync
  - Frame management

- **`ISwapchain`**: Presentation management
  - Acquire next image
  - Present rendered images

- **`IRenderFactory`**: Object creation factory
  - Create devices, buffers, images, pipelines, etc.
  - Abstraction for backend-specific object creation

**Benefits:**
- Rendering code is portable across graphics APIs
- Easier to add new graphics backends
- Cleaner separation between rendering logic and API specifics
- Simplified testing and debugging

### 2. Vulkan Backend Layer

The Vulkan backend implements the rendering interfaces using Vulkan API. This layer has been refactored from a monolithic `BaseClasses.h` into focused, single-responsibility files:

#### File Organization

**VulkanDevice.h** - Device and Queue Management
```cpp
namespace CE {
  class Queues;            // Queue family indices and queues
  class InitializeVulkan;  // Vulkan instance and surface
  class Device;            // Physical/logical device
}
```

**VulkanResources.h** - Buffer and Image Management
```cpp
namespace CE {
  class Buffer;            // GPU buffer wrapper
  class Image;             // Image/texture wrapper
  enum IMAGE_RESOURCE_TYPES; // Image type enumeration
}
```

**VulkanPipeline.h** - Pipeline Management
```cpp
namespace CE {
  struct PushConstants;        // Shader push constants
  class PipelineLayout;        // Pipeline layout wrapper
  class RenderPass;            // Render pass management
  class PipelinesConfiguration; // Pipeline creation/config
  
  // Pipeline state presets (rasterization, blending, etc.)
}
```

**VulkanSync.h** - Synchronization and Swapchain
```cpp
namespace CE {
  struct CommandInterface;      // Command buffer interface
  class CommandBuffers;         // Command buffer management
  class SynchronizationObjects; // Semaphores and fences
  class Swapchain;              // Swapchain management
}
```

**VulkanDescriptor.h** - Descriptor Management
```cpp
namespace CE {
  class DescriptorInterface; // Descriptor set management
  class Descriptor;          // Base descriptor class
}
```

**VulkanUtils.h** - Utility Functions
```cpp
namespace CE {
  template<typename...> void VULKAN_RESULT(); // Error checking
}
```

**BaseClasses.h** - Unified Header
```cpp
// Includes all Vulkan headers for backward compatibility
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanPipeline.h"
#include "VulkanSync.h"
#include "VulkanDescriptor.h"
#include "VulkanUtils.h"
```

#### Benefits of Segmentation

1. **Improved Maintainability**
   - Each file has a clear, focused responsibility
   - Easier to locate and modify specific functionality
   - Reduced cognitive load when working with individual components

2. **Faster Compilation**
   - Include only the headers you need
   - Reduced header dependencies
   - Better incremental build times

3. **Better Organization**
   - Logical grouping of related functionality
   - Clear separation of concerns
   - Self-documenting structure

4. **Easier Testing**
   - Can test individual components in isolation
   - Reduced test setup complexity
   - Better unit test coverage

5. **Backward Compatibility**
   - Existing code continues to work unchanged
   - All code using `BaseClasses.h` still compiles
   - Gradual migration path to new headers

## Application Layer

### VulkanMechanics

Manages the core Vulkan infrastructure:

```cpp
class VulkanMechanics {
  CE::InitializeVulkan initVulkan;
  CE::Queues queues;
  Device mainDevice;
  SynchronizationObjects syncObjects;
  Swapchain swapchain;
};
```

### Resources

Manages GPU resources and descriptors:

```cpp
class Resources {
  CE::ShaderAccess::CommandResources commands;
  CE::CommandInterface commandInterface;
  CE::PushConstants pushConstant;
  World world;
  CE::DescriptorInterface descriptorInterface;
  CE::Image depthImage;
  CE::Image msaaImage;
  UniformBuffer uniform;
  StorageBuffer storage;
  ImageSampler textures;
  StorageImage storageImages;
};
```

### Pipelines

Manages graphics and compute pipelines:

```cpp
class Pipelines {
  ComputeLayout compute;
  GraphicsLayout graphics;
  Render render;
  Configuration config;
};
```

### CapitalEngine

Main engine orchestrator:

```cpp
class CapitalEngine {
  VulkanMechanics mechanics;
  Resources resources;
  Pipelines pipelines;
  
  void mainLoop();
  void drawFrame();
};
```

## Design Patterns

### 1. Inheritance-based Polymorphism

The Vulkan backend uses inheritance to extend base functionality:

```cpp
// Base class in CE namespace
namespace CE {
  class Device { /* ... */ };
}

// Derived class with specific configuration
struct Device : public CE::Device {
  Device() {
    features.tessellationShader = VK_TRUE;
    features.sampleRateShading = VK_TRUE;
    // ...
  }
};
```

### 2. RAII (Resource Acquisition Is Initialization)

All Vulkan resources are managed using RAII:

```cpp
class Buffer {
  ~Buffer() { /* cleanup */ }
};

class Image {
  ~Image() { destroyVulkanImages(); }
};
```

### 3. Factory Pattern

The rendering interface uses a factory pattern for object creation:

```cpp
class IRenderFactory {
  virtual std::unique_ptr<IDevice> createDevice() = 0;
  virtual std::unique_ptr<IBuffer> createBuffer(...) = 0;
  // ...
};
```

### 4. Interface Segregation

Interfaces are kept focused and minimal:

```cpp
class IDevice {
  virtual const char* getDeviceName() const = 0;
  virtual void waitIdle() const = 0;
  // Only device-specific methods
};
```

## Future Enhancements

### Potential Improvements

1. **Multiple Backend Support**
   - Implement OpenGL backend for `RenderInterface`
   - Add DirectX 12 backend
   - Metal backend for macOS/iOS

2. **Enhanced Abstraction**
   - More comprehensive shader abstraction
   - Material system abstraction
   - Render graph abstraction

3. **Performance Optimizations**
   - Command buffer pooling
   - Descriptor set caching
   - Pipeline state object caching

4. **Additional Features**
   - Ray tracing support
   - Mesh shading support
   - Variable rate shading

## Migration Guide

### Using New Headers Directly

Old approach (still works):
```cpp
#include "BaseClasses.h"
```

New approach (more selective):
```cpp
#include "VulkanDevice.h"
#include "VulkanResources.h"
// Only include what you need
```

### Working with Rendering Interface

To write API-agnostic rendering code:

```cpp
#include "RenderInterface.h"

void renderScene(RenderInterface::ICommandBuffer& cmd,
                 RenderInterface::IPipeline& pipeline) {
  cmd.begin();
  cmd.bindPipeline(pipeline);
  cmd.draw(vertexCount);
  cmd.end();
}
```

## Best Practices

1. **Include Only What You Need**
   - Use specific headers (e.g., `VulkanDevice.h`) instead of `BaseClasses.h` when possible
   - Reduces compilation time and dependencies

2. **Use Rendering Interface for New Code**
   - Write new rendering code against `RenderInterface` types
   - Makes code more portable and testable

3. **Follow RAII Principles**
   - Always use smart pointers or RAII wrappers
   - Resources should clean themselves up

4. **Leverage the Factory Pattern**
   - Use `IRenderFactory` to create objects
   - Avoids direct dependency on Vulkan types

## Conclusion

The refactored architecture provides:

- **Better Organization**: Code is logically separated and easier to navigate
- **Improved Maintainability**: Each file has a single, clear responsibility
- **API Independence**: Core rendering logic can work with different graphics APIs
- **Backward Compatibility**: Existing code continues to work without changes
- **Future-Proof Design**: Easy to add new backends and features

This architecture enables the GENERATIONS engine to be more modular, maintainable, and extensible while keeping Vulkan utilities fully accessible.
