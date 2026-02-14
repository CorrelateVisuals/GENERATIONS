# Vendored Library Versions

This file tracks the versions of all third-party libraries vendored in this repository.

## Purpose
- Track exact versions for reproducibility
- Enable security auditing and CVE tracking
- Document upgrade history
- Provide source URLs for updates

## Update Process
1. Check upstream repository for new releases and security advisories
2. Review changelog for breaking changes and new features
3. Download new version from official source
4. Update this file with new version information
5. Test build and runtime functionality
6. Commit with descriptive message: "Update [library] to version [X.Y.Z]"
7. Document any API changes in release notes

---

## Current Versions

### VulkanMemoryAllocator (VMA)
- **File**: `vk_mem_alloc.h`
- **Current Version**: 3.1.0
- **Last Verified**: 2024-02-14
- **Source**: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
- **Latest Upstream**: Check https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/releases
- **License**: MIT
- **Purpose**: Vulkan memory allocation and management
- **Notes**: AMD-maintained, industry standard for Vulkan memory management

**Quarterly Check**:
- [ ] Compare with latest release
- [ ] Review changelog for bug fixes
- [ ] Check for security advisories
- [ ] Update if beneficial

### stb_image
- **File**: `stb_image.h`
- **Current Version**: 2.28
- **Last Verified**: 2024-02-14
- **Source**: https://github.com/nothings/stb
- **Latest Upstream**: Check https://github.com/nothings/stb/blob/master/stb_image.h
- **License**: MIT / Public Domain
- **Purpose**: Image loading (PNG, JPG, BMP, TGA, etc.)
- **Notes**: Part of STB single-file libraries collection, widely used

**Quarterly Check**:
- [ ] Compare with upstream version
- [ ] Review commit history for fixes
- [ ] Test with current usage
- [ ] Update if needed

### stb_image_write
- **File**: `stb_image_write.h`
- **Current Version**: 1.16
- **Last Verified**: 2024-02-14
- **Source**: https://github.com/nothings/stb
- **Latest Upstream**: Check https://github.com/nothings/stb/blob/master/stb_image_write.h
- **License**: MIT / Public Domain
- **Purpose**: Image writing (PNG, BMP, TGA, JPG)
- **Notes**: Part of STB single-file libraries collection

**Quarterly Check**:
- [ ] Compare with upstream version
- [ ] Review commit history for fixes
- [ ] Update if needed

### tiny_obj_loader
- **File**: `tiny_obj_loader.h`
- **Current Version**: 2.0.0
- **Last Verified**: 2024-02-14
- **Source**: https://github.com/tinyobjloader/tinyobjloader
- **Latest Upstream**: Check https://github.com/tinyobjloader/tinyobjloader/releases
- **License**: MIT
- **Purpose**: Wavefront OBJ file parsing
- **Notes**: Header-only library, stable API

**Quarterly Check**:
- [ ] Compare with latest release
- [ ] Review changelog
- [ ] Update if beneficial

---

## External Build Dependencies

These are not vendored but required at build/runtime:

### Vulkan SDK
- **Current Requirement**: Vulkan 1.1 or later
- **Recommended Version**: 1.2.X or later
- **Minimum Version**: 1.1.0 (to be enforced in CMake)
- **Tested Version**: Check CI logs and local testing
- **Source**: https://vulkan.lunarg.com/
- **License**: Various (Apache 2.0 for SDK tools)
- **Purpose**: Vulkan headers, validation layers, shader compiler
- **Platform Notes**:
  - Linux: Package manager (`vulkan-sdk`, `libvulkan-dev`)
  - Windows: LunarG installer
  - macOS: LunarG installer or Homebrew

**Action Required**: 
1. Add version check to CMakeLists.txt
2. Document tested SDK versions
3. Test with both 1.2 and 1.3 SDKs

### GLFW
- **Current Requirement**: Version 3.x
- **Recommended Version**: 3.3.8 or later
- **Minimum Version**: 3.3.0 (to be enforced in CMake)
- **Source**: https://www.glfw.org/
- **License**: zlib/libpng
- **Purpose**: Cross-platform windowing and input
- **Platform Notes**:
  - Linked dynamically via CMake `find_package(glfw3)`
  - System-provided or user-built

**Action Required**: 
1. Specify minimum GLFW version in CMakeLists.txt: `find_package(glfw3 3.3 REQUIRED)`
2. Test with GLFW 3.3.x and 3.4.x
3. Document tested configurations

### Python 3
- **Current Requirement**: Python 3.x (build time only)
- **Recommended Version**: 3.8 or later
- **Minimum Version**: 3.6 (to be enforced in CMake)
- **Purpose**: Build scripts, architecture validation
- **Platform Notes**: Build dependency only, not runtime

**Action Required**: 
1. Specify minimum version in CMakeLists.txt: `find_package(Python3 3.6 REQUIRED COMPONENTS Interpreter)`
2. Test build scripts with Python 3.6+ for compatibility

---

## Deprecation Policy

When a library needs to be replaced:

1. **Evaluation Phase** (1-2 months)
   - Document reasons for deprecation
   - Research alternatives
   - Create proof-of-concept with replacement

2. **Migration Phase** (1-3 months)
   - Implement abstraction layer if needed
   - Gradually migrate to new library
   - Maintain backward compatibility

3. **Cleanup Phase** (1 month)
   - Remove deprecated library
   - Update documentation
   - Verify all references removed

---

## Security Monitoring

### Recommended Tools
- **GitHub Security Advisories**: Monitor repositories for CVEs
- **CVE Databases**: Check NVD (nvd.nist.gov) for vulnerabilities
- **Dependabot**: Consider enabling for automated alerts

### Security Review Schedule
- **Quarterly**: Check for security advisories in all dependencies
- **On CVE Publication**: Immediate assessment if critical
- **Before Releases**: Full security audit of all dependencies

### Vulnerability Response
1. Assess severity and impact
2. Check for patches or workarounds
3. Update library if possible
4. Document any temporary mitigations
5. Notify users if necessary

---

## Version History

### 2024-02-14
- Created initial VERSIONS.md file
- Ran automated version detection script
- Documented current versions:
  - VulkanMemoryAllocator: 3.1.0
  - stb_image: 2.28
  - stb_image_write: 1.16
  - tiny_obj_loader: 2.0.0
- Established update and security monitoring procedures
- Created `tools/check_library_versions.py` for automated version checking

---

## Next Steps

### Completed Actions
- [x] Audit all vendored libraries to determine current versions
- [x] Create automated version checking script (`tools/check_library_versions.py`)
- [x] Document current versions in this file

### Immediate Actions
- [ ] Add minimum Vulkan version check to CMakeLists.txt
- [ ] Add minimum GLFW version check to CMakeLists.txt  
- [ ] Add minimum Python version to CMakeLists.txt
- [ ] Set up quarterly security review schedule (see MAINTENANCE_SCHEDULE.md)

### Future Improvements
- [ ] Check upstream repositories for updates (quarterly)
- [ ] Compare current versions with latest releases
- [ ] Review security advisories for all dependencies
- [ ] Consider migrating to CMake FetchContent for automatic versioning
- [ ] Automate version checking in CI pipeline
- [ ] Create upgrade testing checklist
- [ ] Document performance impact of library updates
