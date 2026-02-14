# Build System Migration Guide

## Current State Analysis

The GENERATIONS project currently maintains **dual build systems**:

1. **CMake** (`CMakeLists.txt`) - Cross-platform, Linux primary
2. **Visual Studio** (`.sln` and `.vcxproj` files) - Windows-specific

This creates a **synchronization burden** and potential for drift between the two systems.

---

## Recommended Migration: CMake as Single Source of Truth

### Benefits

1. **Elimination of Drift**: One build configuration to maintain
2. **Cross-Platform Consistency**: Same build logic on all platforms
3. **Modern IDE Support**: Visual Studio, CLion, VS Code all support CMake
4. **Reduced Maintenance**: Update one file instead of two
5. **Better CI/CD**: Single build system easier to automate
6. **Future-Proof**: CMake supports emerging platforms

### Migration Strategy

#### Phase 1: Preparation (1-2 weeks)

**Step 1: Verify CMake Configuration**

Ensure CMakeLists.txt has all features from Visual Studio project:

```cmake
# Check compiler flags match .vcxproj settings
if(MSVC)
    add_compile_options(
        /W4                 # Warning level 4
        /WX                 # Treat warnings as errors (if desired)
        /permissive-       # Standards conformance
        /Zc:__cplusplus    # Correct __cplusplus macro
    )
    
    # Match Visual Studio optimization/debug settings
    add_compile_options(
        $<$<CONFIG:Debug>:/Od /Zi /MDd>
        $<$<CONFIG:Release>:/O2 /MD>
    )
endif()
```

**Step 2: Test CMake-Generated Visual Studio Projects**

```bash
# Generate Visual Studio 2022 solution
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build_vs

# Open generated solution
start build_vs/GENERATIONS.sln
```

Verify:
- [ ] All source files are included
- [ ] Correct build configurations (Debug/Release)
- [ ] Proper include directories
- [ ] Dependencies link correctly
- [ ] Output paths match expectations

**Step 3: Document Differences**

Compare generated .vcxproj with hand-maintained version:
- Note any missing compiler flags
- Check project dependencies
- Verify post-build steps

#### Phase 2: Migration (1 week)

**Step 1: Enhance CMakeLists.txt**

Add any missing features:

```cmake
# Set output directories to match Visual Studio conventions
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# Per-configuration output directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${PROJECT_SOURCE_DIR}/bin)
endforeach()

# Source file organization for Visual Studio
source_group(TREE "${PROJECT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${CAPITALENGINE_SOURCES})
```

**Step 2: Create Developer Documentation**

Create `docs/BUILDING.md`:

```markdown
# Building GENERATIONS

## Windows (Visual Studio)

### Prerequisites
- Visual Studio 2022 or later
- Vulkan SDK (https://vulkan.lunarg.com/)
- GLFW3 (system install or vcpkg)
- CMake 3.20+

### Generate Visual Studio Project

```bash
# From repository root
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build

# Open solution
start build/GENERATIONS.sln
```

Build using Visual Studio or command line:
```bash
cmake --build build --config Release
```

## Linux

### Prerequisites
- GCC 11+ or Clang 14+
- Vulkan SDK (`libvulkan-dev`)
- GLFW3 (`libglfw3-dev`)
- CMake 3.20+
- Python 3

### Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
./bin/CapitalEngine
```

## macOS

### Prerequisites
- Xcode Command Line Tools
- Vulkan SDK with MoltenVK
- GLFW3 (Homebrew: `brew install glfw`)
- CMake 3.20+

### Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(sysctl -n hw.ncpu)
./bin/CapitalEngine
```
```

**Step 3: Update README.md**

Update build instructions to reference CMake workflow:

```markdown
## Building

See [docs/BUILDING.md](docs/BUILDING.md) for detailed build instructions.

**Quick Start:**

```bash
# Linux/macOS
cmake -S . -B build && cmake --build build

# Windows (Visual Studio)
cmake -G "Visual Studio 17 2022" -S . -B build
cmake --build build --config Release
```
```

**Step 4: Backup Old Build Files**

```bash
# Create archive of old Visual Studio files
mkdir -p archive/old_build_system_$(date +%Y%m%d)
cp GENERATIONS.sln archive/old_build_system_$(date +%Y%m%d)/
cp -r src/*.vcxproj* archive/old_build_system_$(date +%Y%m%d)/
```

#### Phase 3: Transition (1-2 weeks)

**Step 1: Update .gitignore**

Add build directories and generated projects to .gitignore:

```gitignore
# CMake build directories
build/
build_*/
out/

# Generated Visual Studio files (if using CMake generation)
*.sln
*.vcxproj
*.vcxproj.filters
*.vcxproj.user

# Keep only CMakeLists.txt as source of truth
!CMakeLists.txt
```

**Step 2: Remove Hand-Maintained Visual Studio Files**

After confirming CMake-generated projects work:

```bash
git rm GENERATIONS.sln
git rm src/*.vcxproj
git rm src/*.vcxproj.filters
git commit -m "Migrate to CMake-only build system

- Remove hand-maintained Visual Studio solution/project files
- Visual Studio users should now generate projects via CMake
- See docs/BUILDING.md for updated build instructions
"
```

**Step 3: Update Documentation**

Update all references to Visual Studio workflow:
- README.md
- Contributing guidelines (if any)
- Developer onboarding docs

**Step 4: Notify Team**

Send announcement:
```
📢 Build System Update

We've migrated to a CMake-only build system for better maintainability.

Windows/Visual Studio users:
- Run: cmake -G "Visual Studio 17 2022" -S . -B build
- Open: build/GENERATIONS.sln
- Or build via: cmake --build build --config Release

See docs/BUILDING.md for full instructions.
```

#### Phase 4: Validation (1 week)

**Checklist**:
- [ ] CMake generates working Visual Studio projects
- [ ] Linux build works unchanged
- [ ] macOS build works (if supported)
- [ ] All source files are included
- [ ] Shaders compile correctly
- [ ] Dependencies link properly
- [ ] Output paths are correct
- [ ] Debug and Release configurations work
- [ ] Architecture checks still run
- [ ] CI/CD updated to use CMake

---

## CMake Best Practices for Long-Term Maintenance

### 1. Use Modern CMake (3.20+)

**Target-based approach:**
```cmake
# Good: Target-based
add_executable(CapitalEngine ${SOURCES})
target_include_directories(CapitalEngine PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(CapitalEngine PRIVATE glfw Vulkan::Vulkan)

# Avoid: Directory-based
include_directories(${PROJECT_SOURCE_DIR}/src)
link_libraries(glfw vulkan)
```

### 2. Explicit Dependency Management

```cmake
# Find packages with version requirements
find_package(Vulkan 1.2 REQUIRED)

# Check if found
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Install from https://vulkan.lunarg.com/")
endif()

# Report what was found
message(STATUS "Vulkan version: ${Vulkan_VERSION}")
message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIRS}")
```

### 3. Cross-Platform Compiler Flags

```cmake
# Platform-specific flags
if(MSVC)
    target_compile_options(CapitalEngine PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CapitalEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Sanitizers (optional, for development)
if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(CapitalEngine PRIVATE 
            -fsanitize=address,undefined 
            -fno-omit-frame-pointer
        )
        target_link_options(CapitalEngine PRIVATE 
            -fsanitize=address,undefined
        )
    endif()
endif()
```

### 4. Build Type Configuration

```cmake
# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
        "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" 
        FORCE
    )
endif()

# Report build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
```

### 5. Organize Source Files

```cmake
# Collect sources by directory
file(GLOB_RECURSE APP_SOURCES 
    CONFIGURE_DEPENDS
    "src/app/*.cpp" 
    "src/app/*.h"
)

file(GLOB_RECURSE RENDER_SOURCES 
    CONFIGURE_DEPENDS
    "src/render/*.cpp" 
    "src/render/*.h"
)

# Combine
set(ALL_SOURCES ${APP_SOURCES} ${RENDER_SOURCES} ...)

# Create source groups for IDE organization
source_group("App" FILES ${APP_SOURCES})
source_group("Render" FILES ${RENDER_SOURCES})
```

### 6. Custom Targets for Maintenance Tasks

```cmake
# Architecture boundary check
add_custom_target(check_architecture
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_folder_dependencies.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking folder dependency boundaries"
)

# Code formatting
find_program(CLANG_FORMAT NAMES clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Formatting code with clang-format"
    )
endif()

# Make check_architecture run before build
add_dependencies(CapitalEngine check_architecture)
```

---

## Alternative: Coexistence Strategy (Not Recommended)

If full migration is not possible, you can maintain both systems with safeguards:

### Option A: CMake as Primary, VS as Generated

```bash
# Always generate .sln from CMake
cmake -G "Visual Studio 17 2022" -S . -B build_vs

# Commit only CMakeLists.txt
# Add .sln and .vcxproj to .gitignore
# Update on every CMakeLists.txt change
```

**Pros**: 
- Single source of truth (CMake)
- Visual Studio users get familiar .sln workflow

**Cons**:
- Must regenerate on CMake changes
- Extra step in workflow

### Option B: Synchronization Checks

If maintaining both manually:

```python
# tools/check_build_sync.py
# Verify .sln and CMakeLists.txt list same source files
import re

def get_cmake_sources():
    # Parse CMakeLists.txt for source files
    pass

def get_vcxproj_sources():
    # Parse .vcxproj for source files
    pass

if get_cmake_sources() != get_vcxproj_sources():
    print("ERROR: CMakeLists.txt and .vcxproj are out of sync!")
    exit(1)
```

**Pros**: Catches desynchronization early

**Cons**: Extra maintenance, still dual systems

---

## Rollback Plan

If migration encounters issues:

1. **Restore from archive**:
   ```bash
   git revert <migration_commit>
   # Or restore from archive/
   ```

2. **Keep both temporarily**:
   - Continue using hand-maintained .sln/.vcxproj
   - Improve CMake generation in parallel
   - Migrate when ready

3. **Document issues**:
   - What didn't work?
   - What needs to be added to CMake?
   - Timeline for retry

---

## Success Criteria

Migration is successful when:

- [ ] CMake generates working Visual Studio projects
- [ ] All developers can build using CMake workflow
- [ ] CI/CD uses CMake exclusively
- [ ] No build system synchronization needed
- [ ] Build time is comparable or better
- [ ] All features (Debug/Release, warnings, etc.) work
- [ ] Documentation is updated
- [ ] Team is trained on new workflow

---

## Timeline Summary

| Phase | Duration | Activities |
|-------|----------|------------|
| Preparation | 1-2 weeks | Verify CMake, test generation, document differences |
| Migration | 1 week | Enhance CMake, update docs, backup old files |
| Transition | 1-2 weeks | Remove old files, notify team, update workflows |
| Validation | 1 week | Test all platforms, verify all features |
| **Total** | **4-6 weeks** | Full migration with testing |

---

## Post-Migration Maintenance

### Monthly
- [ ] Verify CMake generates correct projects
- [ ] Test on latest Visual Studio version
- [ ] Update build docs if needed

### Quarterly  
- [ ] Test with latest CMake version
- [ ] Review and optimize build configuration
- [ ] Check for new CMake best practices

### Annually
- [ ] Major CMake version compatibility check
- [ ] Build system audit
- [ ] Developer workflow survey

---

**Recommendation**: Proceed with full migration to CMake-only. The long-term benefits (maintainability, consistency, cross-platform support) far outweigh the short-term effort.

**Next Step**: Begin Phase 1 (Preparation) by testing CMake-generated Visual Studio projects.
