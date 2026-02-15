# Performance Optimizations

## Stage Strip Rendering Optimization (February 2026)

### Problem
The stage strip GUI feature was rendering 5 separate viewport tiles every frame, each with multiple draw calls:
- Tile 0: LandscapeDebug (1 draw call)
- Tile 1: LandscapeStage1 (1 draw call)
- Tile 2: LandscapeStage2 (1 draw call)
- Tile 3: Landscape (1 draw call)
- Tile 4: Full scene with Sky, Landscape, TerrainBox, CellsFollower, Cells (5 draw calls)

**Total: 9 additional draw calls per frame**, causing severe performance degradation.

Additionally, the configuration was being recalculated every frame by reading environment variables via `std::getenv()`, which is expensive.

### Solution
Implemented multiple optimization strategies:

#### 1. Environment Variable Caching
- Cached all environment variable reads (`CE_RENDER_STAGE_STRIP`, `CE_RENDER_STAGE_STRIP_HEIGHT`, `CE_RENDER_STAGE_STRIP_PADDING`) on first access
- Uses lazy initialization pattern to avoid startup cost
- Reduces per-frame overhead from 3 `getenv()` calls + parsing to zero

#### 2. Static Label Array
- Changed `get_stage_strip_labels()` to return a const reference to a static array
- Eliminates array allocation and copy on every frame
- Changed from: `std::array<const char *, 5> get_stage_strip_labels()`
- Changed to: `const std::array<const char *, 5>& get_stage_strip_labels()`

#### 3. Early Exit Optimization
- Added `is_stage_strip_enabled()` function for quick enabled check
- Avoids full configuration calculation when stage strip is disabled
- Prevents unnecessary viewport/scissor calculations when not needed

### Performance Impact
When stage strip is **disabled** (recommended for production):
- Saves 9 draw calls per frame
- Eliminates 3 environment variable lookups per frame
- Eliminates extent calculations and viewport/scissor setup

When stage strip is **enabled** (debug/development only):
- Saves 3 environment variable lookups per frame
- Eliminates array allocation per frame
- Still has full rendering functionality

### Usage
The stage strip feature is controlled via environment variables:

```bash
# Disable stage strip entirely (recommended for performance)
export CE_RENDER_STAGE_STRIP=false

# Enable stage strip (default)
export CE_RENDER_STAGE_STRIP=true

# Customize strip height (in pixels)
export CE_RENDER_STAGE_STRIP_HEIGHT=200

# Customize padding (in pixels)
export CE_RENDER_STAGE_STRIP_PADDING=10
```

### Code Changes
- `src/render/gui.h`: Added `<cstdint>`, changed return type, added `is_stage_strip_enabled()`
- `src/render/gui.cpp`: Implemented caching with lazy initialization, static label array
- `src/render/ShaderAccess.cpp`: Added early exit when disabled, use reference for labels

### Recommendations
1. **Disable stage strip in production builds** by setting `CE_RENDER_STAGE_STRIP=false`
2. Only enable during development when debugging rendering stages
3. Consider adding a compile-time flag to completely remove stage strip code in release builds
4. Monitor frame time to verify optimization effectiveness

### Future Optimization Opportunities
1. **Conditional compilation**: Add `#ifdef DEBUG_STAGE_STRIP` to completely remove code in release builds
2. **Instanced rendering**: If stage strip is needed, consider using instanced rendering to reduce draw calls
3. **Command buffer caching**: Cache the stage strip command buffer when configuration doesn't change
4. **Texture-based preview**: Render stages to textures once, then composite them instead of re-rendering every frame
