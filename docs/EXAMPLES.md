# Rendering Interface Examples

This document provides examples of how to use the new rendering interface abstraction layer.

## Example 1: Creating a Buffer (API-Agnostic)

Using the rendering interface, you can create buffers without knowing the underlying graphics API:

```cpp
#include "RenderInterface.h"

void createVertexBuffer(RenderInterface::IRenderFactory& factory,
                       const std::vector<Vertex>& vertices) {
    // Create a vertex buffer using the abstract interface
    auto buffer = factory.createBuffer(
        vertices.size() * sizeof(Vertex),
        RenderInterface::BufferUsage::VERTEX,
        RenderInterface::MemoryProperty::HOST_VISIBLE
    );
    
    // Copy vertex data to the buffer
    void* data = buffer->map();
    memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
    buffer->unmap();
    
    return buffer;
}
```

## Example 2: Recording Draw Commands

Drawing without knowing the specific API:

```cpp
void recordDrawCommands(RenderInterface::ICommandBuffer& cmd,
                       RenderInterface::IPipeline& pipeline,
                       RenderInterface::IBuffer& vertexBuffer,
                       uint32_t vertexCount) {
    cmd.begin();
    cmd.bindPipeline(pipeline);
    cmd.bindVertexBuffer(vertexBuffer);
    cmd.draw(vertexCount);
    cmd.end();
}
```

## Example 3: Current Vulkan-Specific Code (Still Works!)

The existing code continues to work exactly as before:

```cpp
#include "BaseClasses.h"

void createVulkanBuffer() {
    CE::Buffer buffer;
    CE::Buffer::create(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer
    );
}
```

## Example 4: Using Segmented Headers

Instead of including the entire `BaseClasses.h`, you can now include only what you need:

```cpp
// Old way - includes everything
#include "BaseClasses.h"

// New way - include only what you need
#include "VulkanDevice.h"    // For CE::Device, CE::Queues
#include "VulkanResources.h" // For CE::Buffer, CE::Image
```

Benefits:
- Faster compilation
- Clearer dependencies
- Better code organization

## Example 5: Working with Device

```cpp
#include "VulkanDevice.h"

void queryDeviceCapabilities(CE::Device& device) {
    // Check if device supports tessellation
    if (device.features.tessellationShader) {
        std::cout << "Tessellation supported!" << std::endl;
    }
    
    // Get max sample count for MSAA
    VkSampleCountFlagBits maxSamples = device.maxUsableSampleCount;
    
    // Wait for device to finish all operations
    vkDeviceWaitIdle(device.logical);
}
```

## Example 6: Pipeline Creation

```cpp
#include "VulkanPipeline.h"

void createComputePipeline(CE::DescriptorInterface& descriptors) {
    // Create pipeline layout with descriptor sets
    CE::PipelineLayout layout;
    layout.createLayout(descriptors.setLayout);
    
    // Configure pipeline
    CE::PipelinesConfiguration config;
    config.pipelineMap["MyCompute"] = CE::PipelinesConfiguration::Compute{
        .shaders = {"compute.comp"},
        .workGroups = {32, 32, 1}
    };
}
```

## Example 7: Synchronization

```cpp
#include "VulkanSync.h"

void synchronizeFrames(CE::SynchronizationObjects& syncObjects) {
    uint32_t currentFrame = syncObjects.currentFrame;
    
    // Wait for the previous frame to finish
    vkWaitForFences(
        device.logical,
        1,
        &syncObjects.graphicsInFlightFences[currentFrame],
        VK_TRUE,
        UINT64_MAX
    );
    
    // Reset fence for this frame
    vkResetFences(
        device.logical,
        1,
        &syncObjects.graphicsInFlightFences[currentFrame]
    );
}
```

## Example 8: Resource Management

```cpp
#include "VulkanResources.h"

void createDepthBuffer(const VkExtent2D& extent, VkFormat depthFormat) {
    CE::Image depthImage(CE_DEPTH_IMAGE, extent, depthFormat);
    
    // Image is automatically created with proper settings
    // Cleanup happens automatically via RAII
}

void createTexture(const std::string& path) {
    CE::Image texture(path);
    
    // Load texture from file
    VkCommandBuffer cmd = /* get command buffer */;
    VkCommandPool pool = /* get command pool */;
    VkQueue queue = /* get queue */;
    
    texture.loadTexture(path, VK_FORMAT_R8G8B8A8_SRGB, cmd, pool, queue);
}
```

## Example 9: Descriptor Sets

```cpp
#include "VulkanDescriptor.h"

void setupDescriptors() {
    CE::DescriptorInterface descriptors;
    
    // Initialize descriptor sets
    descriptors.initialzeSets();
    
    // Update descriptor sets with buffer/image info
    descriptors.updateSets();
}
```

## Example 10: Error Checking

```cpp
#include "VulkanUtils.h"

void safeVulkanCall() {
    // Automatically checks result and throws on error
    CE::VULKAN_RESULT(
        vkCreateBuffer,
        device.logical,
        &bufferInfo,
        nullptr,
        &buffer.buffer
    );
}
```

## Migration Path

### Phase 1: Current State (All Working)
All existing code using `BaseClasses.h` continues to work without any changes.

### Phase 2: Gradual Optimization (Optional)
Start using segmented headers in new code or when modifying existing files:

```cpp
// Instead of
#include "BaseClasses.h"

// Use specific includes
#include "VulkanDevice.h"
#include "VulkanResources.h"
```

### Phase 3: Future API-Agnostic Code (Optional)
For new features that should be portable:

```cpp
#include "RenderInterface.h"

void portableRenderingCode(RenderInterface::ICommandBuffer& cmd) {
    // This code works with any graphics backend
    cmd.begin();
    // ...
    cmd.end();
}
```

## Best Practices

1. **Use the right level of abstraction**
   - Use `RenderInterface` for truly portable code
   - Use Vulkan classes directly when you need Vulkan-specific features
   - Include only the headers you need

2. **Leverage RAII**
   - All resources clean up automatically
   - Use smart pointers for factory-created objects

3. **Check errors**
   - Use `CE::VULKAN_RESULT` for Vulkan calls
   - Wrap API calls in try-catch blocks

4. **Follow existing patterns**
   - Use the existing inheritance patterns
   - Follow the descriptor/buffer/image setup patterns
   - Maintain the command recording patterns

## Conclusion

The new architecture provides flexibility:
- **Backward compatible**: All existing code works
- **Optimized compilation**: Include only what you need
- **Future-proof**: Can write API-agnostic code
- **Well-organized**: Easy to find and understand components

Choose the approach that best fits your needs!
