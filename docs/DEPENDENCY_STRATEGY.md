# Dependency Management Strategy

## Philosophy

The GENERATIONS project follows a **minimal dependency** philosophy to ensure long-term viability, maintainability, and cross-platform compatibility.

### Core Principles

1. **Minimize External Dependencies**
   - Every dependency is a potential point of failure
   - Fewer dependencies = easier to maintain over decades
   - Prefer standard library solutions when possible

2. **Prefer Industry Standards**
   - Choose technologies with multi-vendor support
   - Avoid proprietary or single-vendor solutions
   - Select standards-body approved APIs (Khronos, ISO, etc.)

3. **Version Lock All Dependencies**
   - Document exact versions for reproducibility
   - Test upgrades thoroughly before adoption
   - Maintain upgrade history for troubleshooting

4. **Regular Security Audits**
   - Quarterly dependency security reviews
   - Monitor CVE databases and security advisories
   - Rapid response to critical vulnerabilities

5. **Graceful Degradation**
   - Don't require bleeding-edge features
   - Provide fallbacks when possible
   - Support a range of versions, not just latest

---

## Dependency Classification

### Tier 1: Critical (Cannot Replace)

These dependencies are fundamental to the application and cannot be removed.

| Dependency | Category | Justification | Replacement Strategy |
|------------|----------|---------------|----------------------|
| **Vulkan API** | Graphics/Compute | Industry standard, HPC requirement | None (core technology) |
| **C++ Standard Library** | Language | Required by C++ | None (language feature) |

**Management**:
- Pin to specific API versions
- Document minimum requirements clearly
- Test on multiple vendors/platforms
- Never use experimental extensions without fallbacks

### Tier 2: Core (Difficult to Replace)

These dependencies are deeply integrated but could theoretically be replaced.

| Dependency | Category | Justification | Replacement Strategy |
|------------|----------|---------------|----------------------|
| **GLFW** | Windowing | Cross-platform window/input | Abstract behind interface, support alternatives (SDL2, native) |
| **CMake** | Build System | Industry standard build tool | Could migrate to alternative build systems if needed |
| **VulkanMemoryAllocator** | Memory Management | Best-in-class Vulkan memory allocator | Abstract behind interface, could implement custom allocator |

**Management**:
- Abstract behind well-defined interfaces
- Document alternatives and migration paths
- Test compatibility with multiple versions
- Prepare abstraction layer for future replacement

### Tier 3: Utility (Easy to Replace)

These dependencies provide utility functions and could be replaced with minimal effort.

| Dependency | Category | Justification | Replacement Strategy |
|------------|----------|---------------|----------------------|
| **stb_image** | Image Loading | Simple, header-only | Many alternatives (SDL_image, libpng, etc.) |
| **stb_image_write** | Image Writing | Simple, header-only | Many alternatives |
| **tiny_obj_loader** | 3D Model Loading | Simple, header-only | Many alternatives (assimp, tinyobjloader-c, custom) |

**Management**:
- Vendor source code (header-only)
- Document version and source URL
- Can swap out with minimal code changes
- Keep abstraction layer thin

### Tier 4: Build-Time Only

These dependencies are only needed during build, not at runtime.

| Dependency | Category | Justification | Replacement Strategy |
|------------|----------|---------------|----------------------|
| **Python 3** | Build Scripts | Architecture validation, tooling | Could migrate to shell scripts if needed |
| **Shader Compiler** | Shader Compilation | GLSL → SPIR-V compilation | Multiple options (glslc, glslangValidator) |

**Management**:
- Document minimum versions
- No runtime dependency
- Keep scripts simple and portable
- Provide fallback/alternative tools

---

## Dependency Selection Criteria

When considering a new dependency, evaluate:

### ✅ Good Dependency Characteristics

1. **Active Maintenance**
   - Regular releases (at least annually)
   - Responsive to security issues
   - Active community or commercial backing

2. **Stability**
   - Mature codebase (5+ years old preferred)
   - Semantic versioning or clear version policy
   - Backward compatibility commitment

3. **Cross-Platform Support**
   - Works on Linux, Windows, macOS at minimum
   - No OS-specific requirements
   - Consistent behavior across platforms

4. **Minimal Dependencies**
   - Few or no transitive dependencies
   - Header-only or static linking preferred
   - No dynamic library version conflicts

5. **Liberal Licensing**
   - MIT, BSD, Apache 2.0, or Public Domain
   - Compatible with commercial use
   - No viral/copyleft requirements (unless acceptable)

6. **Good Documentation**
   - Clear API documentation
   - Migration guides between versions
   - Examples and tutorials

### ❌ Red Flags

1. **Avoid Dependencies That:**
   - Require network access at build/runtime
   - Have complex build requirements
   - Are abandoned or unmaintained (no updates in 2+ years)
   - Have restrictive licenses (GPL, unless isolated)
   - Are only supported on one platform
   - Have unstable APIs (frequent breaking changes)
   - Require specific compiler versions or extensions
   - Bundle unnecessary features (prefer focused libraries)

---

## Version Management

### Semantic Versioning Interpretation

When a dependency uses semantic versioning (MAJOR.MINOR.PATCH):

- **MAJOR**: Breaking changes
  - Evaluate carefully before upgrading
  - May require code changes
  - Document migration path

- **MINOR**: New features, backward compatible
  - Safe to upgrade after testing
  - Test new features if using
  - Update when convenient

- **PATCH**: Bug fixes, backward compatible
  - Upgrade for bug fixes and security
  - Minimal testing required
  - Update regularly

### Version Pinning Strategy

1. **Development**: Use specific version ranges
   - Example: CMake `find_package(GLFW 3.3 REQUIRED)`
   - Allows compatible updates
   - Prevents breaking changes

2. **Releases**: Lock to exact versions
   - Document exact versions used in release
   - Enables reproducible builds
   - Useful for troubleshooting

3. **Testing**: Test version ranges
   - Test with minimum supported version
   - Test with latest stable version
   - Document supported range

### Upgrade Policy

1. **Security Updates**: Immediate
   - Apply security patches ASAP
   - Test minimally for regressions
   - Can skip normal upgrade process for critical CVEs

2. **Bug Fix Updates**: Quarterly
   - Review patch releases each quarter
   - Test in development branch
   - Merge if no regressions

3. **Feature Updates**: Annually
   - Review minor version updates annually
   - Evaluate new features for usefulness
   - Update when beneficial

4. **Major Updates**: As Needed
   - Major versions require planning
   - Create migration branch
   - Test thoroughly before adoption
   - Document breaking changes

---

## Dependency Removal Process

When a dependency becomes problematic (unmaintained, security issues, incompatible):

### Phase 1: Evaluation (1-2 weeks)
1. Document reasons for removal
2. Research alternatives
3. Create comparison matrix
4. Get team consensus

### Phase 2: Abstraction (2-4 weeks)
1. Create abstraction interface
2. Wrap existing dependency
3. Ensure tests pass
4. Review abstraction quality

### Phase 3: Migration (2-6 weeks)
1. Implement alternative backend
2. Test both implementations in parallel
3. Gradually switch over
4. Monitor for issues

### Phase 4: Cleanup (1-2 weeks)
1. Remove old dependency
2. Clean up abstraction if needed
3. Update documentation
4. Remove build configuration

---

## Build System Strategy

### Current: CMake

**Why CMake**:
- Industry standard for C++ projects
- Cross-platform (Windows, Linux, macOS, mobile)
- Strong toolchain integration
- Package management ecosystem (vcpkg, conan)
- 20+ year track record

**Version Requirements**:
- Minimum: 3.20 (provides modern features)
- Target: 3.20-3.25 (widely available)
- Avoid: Bleeding-edge features (hard to adopt)

**Best Practices**:
- Use modern CMake (target-based, not directory-based)
- Avoid global settings where possible
- Use `find_package` for external dependencies
- Generate IDE projects, don't hand-maintain
- Keep CMakeLists.txt readable and well-commented

### Visual Studio Integration

**Current Approach**: Hand-maintained .sln/.vcxproj files

**Recommended Approach**: CMake-generated projects
```bash
# Generate Visual Studio 2022 project
cmake -G "Visual Studio 17 2022" -S . -B build

# Open generated solution
start build/GENERATIONS.sln
```

**Benefits**:
- Single source of truth (CMakeLists.txt)
- No manual synchronization
- Easier to maintain
- Consistent across platforms

---

## External Dependency Alternatives

### Windowing Libraries

| Library | Pros | Cons | Viability |
|---------|------|------|-----------|
| **GLFW** (current) | Minimal, stable, Vulkan-native | Desktop only, slow development | ★★★☆☆ |
| **SDL2** | Active, mobile support, larger ecosystem | More complex, heavier | ★★★★☆ |
| **Native APIs** | No dependencies, full control | Platform-specific code, more work | ★★★☆☆ |

**Recommendation**: Abstract windowing, support GLFW + SDL2

### Memory Allocators

| Library | Pros | Cons | Viability |
|---------|------|------|-----------|
| **VMA** (current) | Best-in-class, widely used, AMD-backed | Vulkan-specific | ★★★★★ |
| **Custom** | Full control, no dependency | Complex, error-prone | ★★☆☆☆ |

**Recommendation**: Keep VMA, it's excellent

### Image Loading

| Library | Pros | Cons | Viability |
|---------|------|------|-----------|
| **stb_image** (current) | Simple, header-only, no dependencies | Limited format support | ★★★★☆ |
| **libpng/libjpeg** | Feature-complete, optimized | Separate libraries per format | ★★★☆☆ |
| **SDL_image** | All formats, if using SDL | SDL dependency | ★★★★☆ |

**Recommendation**: Keep stb_image for simplicity

---

## Monitoring and Maintenance

### Quarterly Tasks (Every 3 Months)

1. **Security Audit**
   - [ ] Check GitHub Security Advisories for each dependency
   - [ ] Search CVE databases (nvd.nist.gov)
   - [ ] Review dependency mailing lists/security pages
   - [ ] Document findings in `docs/security_audit_YYYY-QQ.md`

2. **Version Check**
   - [ ] Check for new stable releases of each dependency
   - [ ] Review changelogs for bug fixes and security patches
   - [ ] Update `libraries/VERSIONS.md` with latest versions
   - [ ] Plan upgrade schedule if needed

3. **Build Testing**
   - [ ] Test build on latest OS versions
   - [ ] Test with latest compiler versions
   - [ ] Test with latest Vulkan SDK
   - [ ] Document any issues

### Annual Tasks (Once per Year)

1. **Dependency Review**
   - [ ] Evaluate each dependency for continued use
   - [ ] Check project health (activity, maintenance status)
   - [ ] Consider alternatives if needed
   - [ ] Update this strategy document

2. **Major Updates**
   - [ ] Plan minor/major version upgrades
   - [ ] Test in development branch
   - [ ] Update documentation
   - [ ] Deploy to production

3. **Technology Radar**
   - [ ] Research new technologies in the space
   - [ ] Evaluate emerging standards
   - [ ] Plan long-term technology strategy
   - [ ] Update viability assessments

---

## Documentation Requirements

Every dependency must have:

1. **In `libraries/VERSIONS.md`**:
   - Current version number
   - Source URL
   - License
   - Purpose
   - Last verification date

2. **In Code**:
   - Version comment in vendored files
   - Include guards and namespace isolation
   - Clear attribution in file headers

3. **In `CMakeLists.txt`**:
   - Version requirements specified
   - Failure messages if not found
   - Optional vs. required clearly marked

4. **In `README.md`**:
   - Build dependencies listed
   - Installation instructions per platform
   - Version requirements clearly stated

---

## Risk Mitigation

### Dependency Failure Scenarios

1. **Maintainer Abandonment**
   - **Detection**: No commits for 12+ months
   - **Response**: Fork and maintain, or find replacement
   - **Prevention**: Choose popular projects with multiple maintainers

2. **Security Vulnerability**
   - **Detection**: CVE published, security advisory
   - **Response**: Patch immediately or find alternative
   - **Prevention**: Regular security audits, monitoring

3. **License Change**
   - **Detection**: New release changes license terms
   - **Response**: Stick with old version or replace
   - **Prevention**: Prefer permissive licenses (MIT, BSD)

4. **Breaking API Changes**
   - **Detection**: Major version bump
   - **Response**: Migrate carefully, test thoroughly
   - **Prevention**: Abstract behind interfaces, test multiple versions

5. **Platform Incompatibility**
   - **Detection**: Doesn't build on target platform
   - **Response**: Patch, contribute fix upstream, or replace
   - **Prevention**: Test on all target platforms regularly

---

## Conclusion

This dependency management strategy ensures GENERATIONS remains:
- **Maintainable**: Clear upgrade paths and documentation
- **Secure**: Regular audits and rapid response to vulnerabilities
- **Portable**: Minimal, cross-platform dependencies
- **Sustainable**: Long-term viability through careful selection

**Next Review**: 2025-02-14 (annual)  
**Last Updated**: 2024-02-14
