# Vulkan Requirements and Platform Compatibility

This document specifies the Vulkan API requirements, platform support, and hardware compatibility for the GENERATIONS engine.

---

## Vulkan API Requirements

### Minimum Vulkan Version
- **Required**: Vulkan 1.1 (minimum)
- **Recommended**: Vulkan 1.2 or later
- **Tested**: Vulkan 1.2, 1.3

**Rationale**: Vulkan 1.1 provides essential features while maintaining broad hardware compatibility. Vulkan 1.2+ features should be optional with runtime detection.

### Required Extensions

The following Vulkan extensions are **mandatory** for this application:

| Extension | Purpose | Since Version | Notes |
|-----------|---------|---------------|-------|
| `VK_KHR_swapchain` | Window presentation | 1.0 | Required for rendering to screen |

**Action Required**: Audit codebase to document all used extensions

### Optional Extensions

These extensions enhance functionality when available:

| Extension | Purpose | Fallback Strategy |
|-----------|---------|-------------------|
| `VK_KHR_timeline_semaphore` | Modern synchronization | Use binary semaphores + fences |
| `VK_KHR_dynamic_rendering` | Simplified rendering | Use render passes |

**Action Required**: Implement runtime extension detection and graceful degradation

### Required Features

Document Vulkan features that must be supported by the physical device:

**Action Required**: Audit `VkPhysicalDeviceFeatures` usage and document requirements

**Example Required Features**:
- Geometry shader (if used)
- Compute shader support
- Dynamic state support
- etc.

---

## Platform Support Matrix

### Current Support Status

| Platform | OS Version | Status | Primary Dev | Notes |
|----------|-----------|--------|-------------|-------|
| **Linux** | Ubuntu 22.04+ | ✅ Tested | Yes | X11 and Wayland |
| **Windows** | 10/11 | ✅ Tested | Yes | Visual Studio 2022 |
| **macOS** | 12+ (Monterey) | ⚠️ Untested | No | Requires MoltenVK |
| **Android** | 10+ | ❌ Unsupported | No | Future consideration |
| **iOS** | 14+ | ❌ Unsupported | No | Future consideration |

**Legend**:
- ✅ Tested: Regularly tested and fully supported
- ⚠️ Untested: Should work but not regularly tested
- ❌ Unsupported: Not currently supported

### Platform-Specific Notes

#### Linux
- **Display Servers**: X11 (primary), Wayland (should work)
- **Package Requirements**:
  - Vulkan drivers: `mesa-vulkan-drivers` (Mesa), proprietary (NVIDIA/AMD)
  - Development: `libvulkan-dev`, `libglfw3-dev`
  - Build tools: `build-essential`, `cmake`, `python3`
- **Shader Compiler**: `glslang-tools` or `shaderc` package
- **Tested Distributions**: Ubuntu 22.04, Fedora 38+ (should work)

#### Windows
- **Minimum Version**: Windows 10 (version 1809 or later)
- **Vulkan Support**: Requires graphics driver with Vulkan support
- **Build Environment**: Visual Studio 2022 or later
- **SDK Requirements**: 
  - Vulkan SDK from LunarG (https://vulkan.lunarg.com/)
  - GLFW (system-installed or bundled)
- **Shader Compiler**: Included in Vulkan SDK (glslc, glslangValidator)

#### macOS
- **Vulkan Implementation**: MoltenVK (Vulkan-on-Metal translation)
- **Installation**: Via Vulkan SDK or Homebrew
- **Limitations**: 
  - Not all Vulkan features supported (Metal limitations)
  - Potential performance overhead (translation layer)
  - Extension support may vary
- **Testing Status**: Needs regular testing
- **Action Required**: Test and document MoltenVK compatibility

---

## Hardware Compatibility

### Minimum Hardware Requirements

**GPU Requirements**:
- **Vulkan Support**: Vulkan 1.1 capable GPU
- **VRAM**: 2GB minimum, 4GB+ recommended
- **Compute**: Compute shader support required
- **Features**: Geometry shaders (if used), tessellation (if used)

**CPU Requirements**:
- **Architecture**: x86_64 or ARM64 (AArch64)
- **Cores**: 2+ cores recommended
- **Memory**: 4GB RAM minimum, 8GB+ recommended

### Tested Hardware Configurations

**Action Required**: Maintain list of tested hardware configurations

| Vendor | GPU Model | Driver Version | OS | Status | Notes |
|--------|-----------|----------------|-----|--------|-------|
| NVIDIA | RTX 3060 | XXX.XX | Linux | ✅ | (example) |
| AMD | RX 6700 XT | XXX.XX | Windows | ✅ | (example) |
| Intel | Arc A750 | XXX.XX | Linux | ⚠️ | (example) |

**Update Process**: Add entries as new hardware is tested

### GPU Vendor-Specific Notes

#### NVIDIA
- **Driver**: Proprietary driver recommended (nvidia-driver-XXX)
- **Vulkan Support**: Excellent, all features supported
- **Performance**: Generally optimal
- **Known Issues**: None currently documented

**Action Required**: Document tested NVIDIA driver versions

#### AMD
- **Driver**: AMDGPU-PRO (Linux) or Adrenalin (Windows)
- **Vulkan Support**: Excellent, native Mesa drivers on Linux
- **Performance**: Generally optimal
- **Known Issues**: None currently documented

**Action Required**: Document tested AMD driver versions

#### Intel
- **Driver**: Mesa (Linux), Intel Graphics Driver (Windows)
- **Vulkan Support**: Good on newer hardware (Arc series, 12th gen+)
- **Performance**: Varies by hardware generation
- **Known Issues**: Older integrated GPUs may have limited feature support

**Action Required**: Test on Intel Arc and document compatibility

#### Apple Silicon (M1/M2/M3)
- **Implementation**: MoltenVK (Vulkan-on-Metal)
- **Support Status**: Untested
- **Potential Issues**: 
  - Feature support limitations
  - Translation layer overhead
  - Extension availability

**Action Required**: Test on Apple Silicon and document findings

---

## Vulkan Feature Detection (Recommended Implementation)

To ensure cross-platform compatibility and graceful degradation, implement runtime feature detection:

```cpp
// Example: Detecting optional features
struct VulkanCapabilities {
    bool timelineSemaphores = false;
    bool dynamicRendering = false;
    bool geometryShader = false;
    uint32_t maxComputeWorkGroupSize[3] = {0, 0, 0};
    
    static VulkanCapabilities detect(VkPhysicalDevice device, VkInstance instance) {
        VulkanCapabilities caps;
        
        // Check extensions
        caps.timelineSemaphores = checkExtensionSupport(device, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        caps.dynamicRendering = checkExtensionSupport(device, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        
        // Check features
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        caps.geometryShader = features.geometryShader;
        
        // Check limits
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        memcpy(caps.maxComputeWorkGroupSize, 
               props.limits.maxComputeWorkGroupSize, 
               sizeof(caps.maxComputeWorkGroupSize));
        
        return caps;
    }
};

// Usage example
VulkanCapabilities caps = VulkanCapabilities::detect(physicalDevice, instance);
if (caps.timelineSemaphores) {
    // Use modern synchronization
} else {
    // Fall back to binary semaphores
}
```

**Action Required**: Implement capability detection system

---

## Shader Compatibility

### GLSL Version
- **Target**: GLSL 450 (Vulkan 1.0 compatible)
- **Avoid**: GLSL extensions unless necessary
- **Rationale**: Maximum compatibility across vendors and platforms

### SPIR-V Version
- **Target**: SPIR-V 1.0 or 1.3
- **Compiler**: glslc (preferred) or glslangValidator
- **Validation**: Always validate with `spirv-val`

### Shader Compiler Requirements

| Compiler | Minimum Version | Platform | Notes |
|----------|----------------|----------|-------|
| glslc | 2019.0 | All | Part of shaderc, preferred |
| glslangValidator | 7.x | All | Legacy, still supported |

**Action Required**: Version-lock shader compiler in CI/builds

### Precision and Portability

Different GPU vendors handle shader precision differently:

1. **Float precision**: 
   - Use `highp` explicitly for critical calculations
   - Test on multiple vendors (NVIDIA, AMD, Intel)
   - Document any vendor-specific workarounds

2. **Integer operations**:
   - Specify bit widths explicitly (int32, uint64)
   - Avoid relying on undefined behavior

3. **Shader compilation**:
   - Always compile with warnings enabled
   - Validate SPIR-V with `spirv-val`
   - Test compiled shaders on multiple vendors

**Action Required**: Add shader validation to build/CI process

---

## Validation Layers

### Development Builds
- **Enabled**: Always use validation layers in development
- **Layer**: `VK_LAYER_KHRONOS_validation`
- **Configuration**: Enable all checks, verbose output

### Release Builds
- **Disabled**: Validation layers off for performance
- **Mechanism**: Runtime environment variable or build flag
- **Recommendation**: Provide `--validate` flag for debugging

### Best Practices
```cpp
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// Or: Runtime control via environment variable
const bool enableValidationLayers = std::getenv("ENABLE_VK_VALIDATION") != nullptr;
```

---

## Cross-Platform Build Configuration

### Platform Detection

CMake provides reliable platform detection:

```cmake
if(WIN32)
    # Windows-specific configuration
elseif(APPLE)
    # macOS-specific configuration
    # May need to link MoltenVK
elseif(UNIX)
    # Linux/BSD-specific configuration
endif()
```

### Vulkan SDK Detection

```cmake
find_package(Vulkan REQUIRED)

if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK Version: ${Vulkan_VERSION}")
    message(STATUS "Vulkan Include Dir: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan Libraries: ${Vulkan_LIBRARIES}")
else()
    message(FATAL_ERROR "Vulkan SDK not found. Please install from https://vulkan.lunarg.com/")
endif()

# Require minimum version
if(Vulkan_VERSION VERSION_LESS "1.2.0")
    message(WARNING "Vulkan SDK version ${Vulkan_VERSION} is older than recommended (1.2.0+)")
endif()
```

**Action Required**: Add Vulkan version checking to CMakeLists.txt

---

## Testing Strategy

### Pre-Release Testing Checklist

- [ ] Test on NVIDIA GPU (latest driver)
- [ ] Test on AMD GPU (latest driver)  
- [ ] Test on Intel GPU (if available)
- [ ] Test on Linux (Ubuntu LTS)
- [ ] Test on Windows 10/11
- [ ] Test on macOS with MoltenVK (if targeting)
- [ ] Test with Vulkan 1.1 compatible driver
- [ ] Test with Vulkan 1.2+ driver
- [ ] Run with validation layers enabled (no errors)
- [ ] Verify shader compilation on all platforms
- [ ] Test with integrated GPU (lower capabilities)
- [ ] Test on older GPU (minimum spec)

### Continuous Integration

**Recommended CI Platforms**:
- **Linux**: Ubuntu 22.04 with Mesa drivers
- **Windows**: Windows Server 2022 with software rendering
- **macOS**: Latest macOS with MoltenVK

**CI Limitations**:
- Most CI runners don't have GPU access
- Can test compilation and API usage
- Cannot test rendering correctness
- Use validation layers to catch errors

**Action Required**: Set up GitHub Actions CI for multi-platform builds

---

## Known Platform Limitations

### MoltenVK (macOS)
- Not all Vulkan features are supported
- Some extensions unavailable due to Metal limitations
- Performance overhead from translation layer
- Geometry shaders not supported
- Tessellation support limited

**Mitigation**: Detect capabilities at runtime, provide fallbacks

### Mesa Drivers (Linux)
- Performance varies by hardware
- Some older GPUs have limited feature support
- Regular driver updates recommended

### Integrated GPUs
- Lower performance than discrete GPUs
- May have lower resource limits
- Shared memory with CPU

**Mitigation**: 
- Test with low-end hardware
- Implement quality settings
- Respect device limits

---

## Future Considerations

### Vulkan 1.3+ Features

Monitor these for potential adoption:

- **Dynamic Rendering**: Simplifies render pass management
- **Synchronization2**: Better synchronization primitives
- **Extended Dynamic State**: More flexibility in pipelines
- **Maintenance Extensions**: Various improvements

**Strategy**: 
- Use as optional features with fallbacks
- Require Vulkan 1.2 for now
- Consider Vulkan 1.3 requirement in 2025+

### Mobile Platforms

If targeting Android/iOS in the future:

- **Android**: Native Vulkan support (API level 24+)
- **iOS**: Requires MoltenVK (similar to macOS)
- **Considerations**: Touch input, mobile GPUs, thermal constraints

### Ray Tracing

If considering ray tracing (future HPC workloads):

- Requires `VK_KHR_ray_tracing_pipeline` extension
- Limited to high-end GPUs (NVIDIA RTX 20xx+, AMD RX 6000+)
- Would need optional feature with fallback

---

## Action Items

### Immediate (Critical for Maintainability)

1. [ ] Audit current Vulkan API usage
2. [ ] Document required Vulkan version in code
3. [ ] List all required/optional extensions
4. [ ] Document required device features
5. [ ] Add Vulkan version check to CMakeLists.txt
6. [ ] Implement runtime capability detection

### Short-Term (Within 3 Months)

1. [ ] Test on AMD hardware
2. [ ] Test on Intel hardware  
3. [ ] Test on older Vulkan 1.1 driver
4. [ ] Add shader validation to build process
5. [ ] Document tested hardware configurations
6. [ ] Set up basic CI for multi-platform builds

### Long-Term (Within 1 Year)

1. [ ] Test on macOS with MoltenVK
2. [ ] Add comprehensive platform testing
3. [ ] Implement graceful degradation for missing features
4. [ ] Create hardware compatibility matrix
5. [ ] Add performance testing on various hardware
6. [ ] Document vendor-specific quirks/workarounds

---

## References

- **Vulkan Specification**: https://registry.khronos.org/vulkan/
- **Vulkan SDK**: https://vulkan.lunarg.com/
- **MoltenVK**: https://github.com/KhronosGroup/MoltenVK
- **Mesa Drivers**: https://docs.mesa3d.org/
- **Validation Layers**: https://vulkan.lunarg.com/doc/sdk/latest/windows/khronos_validation_layer.html

---

**Last Updated**: 2024-02-14  
**Next Review**: 2025-02-14 or on Vulkan spec updates  
**Maintainer**: Project team
