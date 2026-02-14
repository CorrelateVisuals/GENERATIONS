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
- **Current Version**: Unknown (needs audit)
- **Last Verified**: Never
- **Source**: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
- **License**: MIT
- **Purpose**: Vulkan memory allocation and management
- **Notes**: Need to add version comment to header file

**Action Required**: 
1. Determine current version by checking header content
2. Compare with latest stable release
3. Add version comment to file header
4. Update this table

### stb_image
- **File**: `stb_image.h`
- **Current Version**: Unknown (needs audit)
- **Last Verified**: Never
- **Source**: https://github.com/nothings/stb
- **License**: MIT / Public Domain
- **Purpose**: Image loading (PNG, JPG, BMP, TGA, etc.)
- **Notes**: Part of STB single-file libraries collection

**Action Required**:
1. Check version in file header
2. Compare with upstream
3. Update this table

### stb_image_write
- **File**: `stb_image_write.h`
- **Current Version**: Unknown (needs audit)
- **Last Verified**: Never
- **Source**: https://github.com/nothings/stb
- **License**: MIT / Public Domain
- **Purpose**: Image writing (PNG, BMP, TGA, JPG)
- **Notes**: Part of STB single-file libraries collection

**Action Required**:
1. Check version in file header
2. Compare with upstream
3. Update this table

### tiny_obj_loader
- **File**: `tiny_obj_loader.h`
- **Current Version**: Unknown (needs audit)
- **Last Verified**: Never
- **Source**: https://github.com/tinyobjloader/tinyobjloader
- **License**: MIT
- **Purpose**: Wavefront OBJ file parsing
- **Notes**: Header-only library

**Action Required**:
1. Check version in file header
2. Compare with upstream
3. Update this table

---

## External Build Dependencies

These are not vendored but required at build/runtime:

### Vulkan SDK
- **Current Requirement**: Any recent version (needs specification)
- **Recommended Version**: 1.3.X or later
- **Minimum Version**: Unknown (needs documentation)
- **Source**: https://vulkan.lunarg.com/
- **License**: Various (Apache 2.0 for SDK tools)
- **Purpose**: Vulkan headers, validation layers, shader compiler
- **Platform Notes**:
  - Linux: Package manager (`vulkan-sdk`, `libvulkan-dev`)
  - Windows: LunarG installer
  - macOS: LunarG installer or Homebrew

**Action Required**: Document minimum and tested versions

### GLFW
- **Current Requirement**: Version 3.x (needs specification)
- **Recommended Version**: 3.3.8 or later
- **Source**: https://www.glfw.org/
- **License**: zlib/libpng
- **Purpose**: Cross-platform windowing and input
- **Platform Notes**:
  - Linked dynamically via CMake `find_package(glfw3)`
  - System-provided or user-built

**Action Required**: 
1. Specify minimum GLFW version in CMakeLists.txt
2. Test with specific versions
3. Document tested configurations

### Python 3
- **Current Requirement**: Python 3.x (build time only)
- **Recommended Version**: 3.8 or later
- **Purpose**: Build scripts, architecture validation
- **Platform Notes**: Build dependency only, not runtime

**Action Required**: Specify minimum version in CMakeLists.txt

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
- Identified need for version audit of all vendored libraries
- Established update and security monitoring procedures

---

## Next Steps

### Immediate Actions
1. [ ] Audit all vendored libraries to determine current versions
2. [ ] Add version information to library file headers
3. [ ] Document minimum external dependency versions
4. [ ] Set up quarterly security review schedule

### Future Improvements
1. [ ] Consider migrating to CMake FetchContent for versioned dependencies
2. [ ] Automate version checking in CI
3. [ ] Create upgrade testing checklist
4. [ ] Document performance impact of library updates
