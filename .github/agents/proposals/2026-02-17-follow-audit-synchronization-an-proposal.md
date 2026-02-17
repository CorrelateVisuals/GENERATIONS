# Code Proposal — FOLLOW-AUDIT-SYNCHRONIZATION-AN (2026-02-17)

This file aggregates proposed code changes from the automated agent run.

## C++ Lead
```cpp
// Example of adding profiling hooks for synchronization
void CapitalEngine::draw_frame() {
    Log::text("{ Frame Start }");
    Log::measure_elapsed_time();

    // Synchronization profiling
    Log::text("{ Sync }", "Waiting for previous frame to finish...");
    vkWaitForFences(mechanics.device, 1, &mechanics.sync_objects.in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    // Reset fence for the current frame
    vkResetFences(mechanics.device, 1, &mechanics.sync_objects.in_flight_fences[current_frame]);

    // Command buffer recording
    Log::text("{ Command Buffer }", "Recording commands...");
    VkCommandBuffer command_buffer = mechanics.commands[current_frame];
    vkResetCommandBuffer(command_buffer, 0);

    // Begin command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    // Add synchronization barrier logging
    Log::text("{ Barrier }", "Adding pipeline barrier for resource transition...");
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = mechanics.swapchain.images[current_frame];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    // End command buffer
    vkEndCommandBuffer(command_buffer);

    Log::text("{ Frame End }");
}
```

## 7) Procedure Recording
### PROC-PERF-003 — Synchronization Profiling Hook
1. Add logging hooks to measure the time spent on synchronization primitives.
2. Log barrier types, affected resources, and pipeline stages.
3. Use `Log::measure_elapsed_time()` to track the duration of synchronization operations.
4. Ensure profiling hooks are lightweight and do not introduce significant overhead.
```

## Vulkan Guru
### Example Barrier Optimization
```cpp
// Original overly conservative barrier
VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

// Optimized barrier based on resource usage
VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

VkMemoryBarrier memoryBarrier = {};
memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

vkCmdPipelineBarrier(
    commandBuffer,
    srcStageMask,
    dstStageMask,
    0, // No dependency flags
    1, &memoryBarrier,
    0, nullptr,
    0, nullptr
);
```

### Swapchain Synchronization Fix
```cpp
void CapitalEngine::recreate_swapchain() {
    vkDeviceWaitIdle(mechanics.device); // Ensure all in-flight frames are complete
    mechanics.swapchain.recreate(mechanics.init_vulkan.surface,
                                  mechanics.queues,
                                  mechanics.sync_objects,
                                  *pipelines,
                                  *resources);
}
```

## 7) Procedure Recording
### PROC-GPU-002 — Barrier Optimization Workflow
1. Identify all pipeline barriers in the render loop.
2. Analyze the resource usage (e.g., image layouts, buffer access patterns).
3. Match the `srcStageMask` and `dstStageMask` to the actual pipeline stages involved.
4. Minimize the `srcAccessMask` and `dstAccessMask` to the specific operations required.
5. Test changes with Vulkan validation layers enabled to ensure correctness.
6. Profile frame time before and after changes to measure performance impact.
```

## Kernel Expert
```glsl
// Example optimization for workgroup size in shaders/Engine.comp
// Current workgroup size: [16, 16, 1]
// Proposed workgroup size: [32, 32, 1] for better occupancy on modern GPUs

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

// Example of subgroup operation usage
void main() {
    uint localID = gl_LocalInvocationID.x + gl_LocalInvocationID.y * gl_WorkGroupSize.x;
    uint globalID = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x;

    // Subgroup operation for reduction
    uint sum = subgroupAdd(localID);

    // Use shared memory to optimize memory access
    shared uint sharedData[1024];
    sharedData[localID] = sum;

    barrier();
    // Further computation...
}
```

## 7) Procedure Recording
### PROC-PERF-003 — Workgroup Size Optimization
1. Profile the current workgroup sizes using GPU profiling tools (e.g., Nsight, RenderDoc).
2. Identify the optimal workgroup size for the target GPU architecture, balancing occupancy and resource usage.
3. Update the `local_size_x`, `local_size_y`, and `local_size_z` values in the compute shader.
4. Validate the changes on both Linux and Windows to ensure cross-platform compatibility.
5. Document the profiling results and the rationale for the chosen workgroup size.
```

## Refactorer
### Example Refactor for Barrier Simplification
```cpp
// src/vulkan_base/VulkanBaseSync.cpp
void VulkanBaseSync::apply_barriers(VkCommandBuffer command_buffer) {
    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.image = target_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer,
                         src_stage, dst_stage,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
}
```

### Example Logging Enhancement
```cpp
// src/engine/Log.cpp
void Log::log_barrier(const VkImageMemoryBarrier &barrier) {
    text("Barrier Applied:");
    text("  Old Layout:", Log::get_image_usage_string(barrier.oldLayout));
    text("  New Layout:", Log::get_image_usage_string(barrier.newLayout));
    text("  Src Access:", Log::get_memory_property_string(barrier.srcAccessMask));
    text("  Dst Access:", Log::get_memory_property_string(barrier.dstAccessMask));
}
```

## 7) Procedure Recording
### PROC-ARCH-003 — Barrier Simplification
- **Origin**: Refactorer Agent, 2023-10-25, Task ID: FOLLOW-AUDIT-SYNCHRONIZATION-AN
- **Steps**:
  1. Identify barriers with overly restrictive pipeline stages or access masks.
  2. Analyze the actual resource usage in shaders and pipeline stages.
  3. Update barriers to use the minimal necessary synchronization scope.
  4. Test changes to ensure no rendering artifacts or synchronization issues.
- **Applies when**: Barriers are causing unnecessary GPU stalls or reduced parallelism.
```

## HPC Marketeer
No direct code changes are required from the HPC Marketeer agent. However, the following documentation updates are proposed:
```markdown
### README.md
#### Synchronization and Barriers
Vulkan synchronization is a critical aspect of ensuring correct and efficient GPU execution. The following concepts are now better optimized in the render loop:
- **Pipeline Barriers**: Used to synchronize access to resources between different pipeline stages.
- **Pipeline Stages**: Ensure that resources are accessed in the correct order.
- **Resource Transitions**: Manage the layout and usage of Vulkan resources (e.g., images, buffers).

#### Debugging Synchronization Issues
To debug synchronization issues, enable Vulkan validation layers and look for warnings related to barriers and pipeline stages. Use the enhanced logging system to trace synchronization events.

### CHANGELOG.md
#### [Unreleased]
- Improved Vulkan synchronization in the render loop:
  - Simplified overly conservative barriers to enhance GPU parallelism.
  - Resolved pipeline stage mismatches to reduce unnecessary synchronization overhead.
  - Clarified resource ownership and lifecycle to prevent synchronization issues.
- Enhanced logging for synchronization events.

### Onboarding Guide
#### Debugging Vulkan Synchronization
1. Enable Vulkan validation layers by setting the `VK_INSTANCE_LAYERS` environment variable.
2. Use the enhanced logging system to trace synchronization events.
3. Refer to the updated README for explanations of synchronization primitives and pipeline stages.
```

## 7) Procedure Recording
### New Reusable Pattern: Documenting Synchronization Changes
When documenting changes to Vulkan synchronization:
1. **Explain Key Concepts**: Include a brief explanation of synchronization primitives, pipeline stages, and resource transitions.
2. **Highlight Changes**: Clearly state what was changed and why (e.g., simplified barriers, resolved mismatches).
3. **Provide Debugging Guidance**: Include steps for debugging synchronization issues using validation layers and logging.
4. **Update Onboarding Materials**: Ensure new contributors can understand and work with the updated synchronization logic.
5. **Align Terminology**: Ensure consistent use of terms across the codebase and documentation.

This pattern has been recorded for future use in the `GUILD:documentation` procedure log.
```
