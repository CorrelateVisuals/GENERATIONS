# Render Picking Implementation Guide

This document demonstrates various approaches to implementing render picking in the GENERATIONS engine, enabling the identification of which cell was clicked and changing its color.

## Overview

Render picking is the process of determining which 3D object was selected by the user when they click on a 2D screen coordinate. The GENERATIONS engine supports multiple approaches to this problem.

## Method 1: GPU-Based ID Picking (Recommended)

**Concept:** Render each cell with a unique ID to an offscreen buffer, then read back the pixel at the mouse position to determine which cell was clicked.

**Advantages:**
- Very accurate for complex geometry
- Handles overlapping objects correctly
- GPU-accelerated
- Works well with instanced rendering

**Disadvantages:**
- Requires additional render pass
- Small GPU memory overhead for ID buffer
- Requires buffer readback (minor latency)

### Implementation Steps:

1. **Create ID Render Target**
   ```cpp
   // In Resources class, create an offscreen render target
   VkImage pickingImage;
   VkImageView pickingImageView;
   VkFramebuffer pickingFramebuffer;
   ```

2. **Add Picking Shaders**
   - **Vertex Shader (CellsPicking.vert):** Pass through positions and instance ID
   - **Fragment Shader (CellsPicking.frag):** Output instance ID as color
   ```glsl
   // CellsPicking.frag
   #version 450
   layout(location = 0) in flat uint instanceID;
   layout(location = 0) out uvec4 outID;
   
   void main() {
       outID = uvec4(instanceID, 0, 0, 0);
   }
   ```

3. **Render Picking Pass**
   ```cpp
   void render_picking_pass() {
       // Bind picking framebuffer
       // Render all pickable objects with ID shader
       // Read pixel at mouse position
   }
   ```

4. **Read Pixel and Identify Cell**
   ```cpp
   uint32_t read_picked_id(int x, int y) {
       // Map picking buffer memory
       // Read pixel at (x, y)
       // Return cell ID
   }
   ```

## Method 2: CPU Ray-Casting

**Concept:** Cast a ray from the camera through the mouse position and test for intersection with cell geometry on the CPU.

**Advantages:**
- No additional GPU resources needed
- Immediate results (no buffer readback)
- Good for simple geometry
- Educational - demonstrates ray-triangle intersection

**Disadvantages:**
- CPU overhead for complex scenes
- Needs accurate world-space geometry
- Must handle transformations manually

### Implementation Steps:

1. **Convert Screen to World Ray**
   ```cpp
   struct Ray {
       glm::vec3 origin;
       glm::vec3 direction;
   };
   
   Ray screen_to_world_ray(float screenX, float screenY, 
                           const glm::mat4& view, 
                           const glm::mat4& projection,
                           int screenWidth, int screenHeight) {
       // Convert screen coords to NDC
       float x = (2.0f * screenX) / screenWidth - 1.0f;
       float y = 1.0f - (2.0f * screenY) / screenHeight;
       
       // NDC to world space
       glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
       glm::vec4 rayEye = glm::inverse(projection) * rayClip;
       rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
       glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
       
       Ray ray;
       ray.origin = glm::vec3(glm::inverse(view) * glm::vec4(0, 0, 0, 1));
       ray.direction = glm::normalize(rayWorld);
       return ray;
   }
   ```

2. **Ray-AABB Intersection Test**
   ```cpp
   bool ray_aabb_intersection(const Ray& ray, 
                              const glm::vec3& aabbMin, 
                              const glm::vec3& aabbMax,
                              float& tMin) {
       glm::vec3 invDir = 1.0f / ray.direction;
       glm::vec3 t0s = (aabbMin - ray.origin) * invDir;
       glm::vec3 t1s = (aabbMax - ray.origin) * invDir;
       
       glm::vec3 tsmaller = glm::min(t0s, t1s);
       glm::vec3 tbigger = glm::max(t0s, t1s);
       
       tMin = glm::max(tsmaller.x, glm::max(tsmaller.y, tsmaller.z));
       float tmax = glm::min(tbigger.x, glm::min(tbigger.y, tbigger.z));
       
       return tMin <= tmax && tmax >= 0.0f;
   }
   ```

3. **Find Closest Cell**
   ```cpp
   int find_clicked_cell(const Ray& ray, const World::Grid& grid) {
       int closestCell = -1;
       float closestDist = FLT_MAX;
       
       for (size_t i = 0; i < grid.cells.size(); ++i) {
           glm::vec3 cellPos = glm::vec3(grid.cells[i].instance_position);
           float cellSize = /* cell size */;
           
           glm::vec3 aabbMin = cellPos - glm::vec3(cellSize * 0.5f);
           glm::vec3 aabbMax = cellPos + glm::vec3(cellSize * 0.5f);
           
           float t;
           if (ray_aabb_intersection(ray, aabbMin, aabbMax, t)) {
               if (t < closestDist) {
                   closestDist = t;
                   closestCell = i;
               }
           }
       }
       return closestCell;
   }
   ```

## Method 3: Depth Buffer + Position Reconstruction

**Concept:** Read the depth buffer at the mouse position and reconstruct the 3D world position, then find the nearest cell.

**Advantages:**
- Uses existing depth buffer
- No extra render passes
- Fast for regular grids

**Disadvantages:**
- Requires depth buffer readback
- Needs accurate position reconstruction
- Ambiguous for overlapping geometry

### Implementation Steps:

1. **Read Depth Value**
   ```cpp
   float read_depth_at_pixel(int x, int y) {
       // Map depth buffer
       // Read depth value at (x, y)
       return depth;
   }
   ```

2. **Reconstruct World Position**
   ```cpp
   glm::vec3 reconstruct_world_pos(float screenX, float screenY, 
                                   float depth,
                                   const glm::mat4& invViewProj) {
       float x = (2.0f * screenX) / screenWidth - 1.0f;
       float y = 1.0f - (2.0f * screenY) / screenHeight;
       float z = depth * 2.0f - 1.0f; // OpenGL depth
       
       glm::vec4 clipPos = glm::vec4(x, y, z, 1.0f);
       glm::vec4 worldPos = invViewProj * clipPos;
       return glm::vec3(worldPos) / worldPos.w;
   }
   ```

3. **Find Nearest Cell**
   ```cpp
   int find_cell_at_position(const glm::vec3& worldPos, 
                             const World::Grid& grid) {
       int closestCell = -1;
       float closestDist = FLT_MAX;
       
       for (size_t i = 0; i < grid.cells.size(); ++i) {
           glm::vec3 cellPos = glm::vec3(grid.cells[i].instance_position);
           float dist = glm::distance(worldPos, cellPos);
           
           if (dist < closestDist) {
               closestDist = dist;
               closestCell = i;
           }
       }
       return closestCell;
   }
   ```

## Method 4: Grid-Based Picking (Optimized for Regular Grids)

**Concept:** For a regular grid like in GENERATIONS, use mathematical projection to directly calculate grid coordinates.

**Advantages:**
- Extremely fast - O(1) lookup
- No GPU readback needed
- Perfect for regular grids
- Minimal CPU overhead

**Disadvantages:**
- Only works for flat, regular grids
- Doesn't account for terrain height variation
- Limited to planar surfaces

### Implementation Steps:

1. **Project Mouse to Grid Plane**
   ```cpp
   struct GridPickResult {
       bool hit;
       int cellX;
       int cellY;
       int cellIndex;
   };
   
   GridPickResult pick_grid_cell(float mouseX, float mouseY,
                                 const glm::mat4& view,
                                 const glm::mat4& projection,
                                 const World::Grid& grid,
                                 float gridY = 0.0f) {
       // Create ray from camera through mouse
       Ray ray = screen_to_world_ray(mouseX, mouseY, view, projection,
                                     screenWidth, screenHeight);
       
       // Intersect with grid plane (y = gridY)
       float t = (gridY - ray.origin.y) / ray.direction.y;
       
       GridPickResult result;
       if (t < 0.0f) {
           result.hit = false;
           return result;
       }
       
       glm::vec3 hitPoint = ray.origin + ray.direction * t;
       
       // Convert world position to grid coordinates
       float cellSize = /* get from grid */;
       int gridX = static_cast<int>((hitPoint.x / cellSize) + grid.size.x / 2);
       int gridZ = static_cast<int>((hitPoint.z / cellSize) + grid.size.y / 2);
       
       result.hit = (gridX >= 0 && gridX < grid.size.x && 
                     gridZ >= 0 && gridZ < grid.size.y);
       result.cellX = gridX;
       result.cellY = gridZ;
       result.cellIndex = result.hit ? (gridZ * grid.size.x + gridX) : -1;
       
       return result;
   }
   ```

## Changing Cell Color

Once a cell is identified, changing its color is straightforward:

```cpp
void change_cell_color(World::Grid& grid, int cellIndex, 
                       const glm::vec4& newColor) {
    if (cellIndex >= 0 && cellIndex < grid.cells.size()) {
        grid.cells[cellIndex].color = newColor;
        
        // Update GPU buffer
        void* data;
        vkMapMemory(device, cellsBuffer.memory, 0, 
                   sizeof(World::Cell) * grid.cells.size(), 0, &data);
        memcpy(data, grid.cells.data(), 
              sizeof(World::Cell) * grid.cells.size());
        vkUnmapMemory(device, cellsBuffer.memory);
    }
}
```

## Integration Example

Here's how to integrate picking into the main loop:

```cpp
void handle_mouse_click(float mouseX, float mouseY) {
    // Method 1: GPU ID Picking
    // uint32_t cellID = read_picked_id(mouseX, mouseY);
    
    // Method 2: Ray Casting
    // Ray ray = screen_to_world_ray(mouseX, mouseY, ...);
    // int cellID = find_clicked_cell(ray, grid);
    
    // Method 4: Grid-based (recommended for GENERATIONS)
    GridPickResult result = pick_grid_cell(mouseX, mouseY, 
                                          world.camera.view,
                                          world.camera.projection,
                                          world.grid);
    
    if (result.hit) {
        Log::text("Clicked cell at grid position:", 
                 result.cellX, ",", result.cellY);
        
        // Change color to highlight
        glm::vec4 highlightColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        change_cell_color(world.grid, result.cellIndex, highlightColor);
    }
}
```

## Recommended Approach for GENERATIONS

For the GENERATIONS engine with its regular grid structure, **Method 4 (Grid-Based Picking)** is recommended as the primary approach because:

1. It's extremely fast and efficient
2. No GPU resources or readback needed
3. Perfect match for the regular cellular grid
4. Can be enhanced with terrain height if needed

For more complex future scenarios (irregular geometry, overlapping objects), **Method 1 (GPU ID Picking)** provides the most robust solution.

## Performance Considerations

| Method | CPU Cost | GPU Cost | Memory | Latency | Best For |
|--------|----------|----------|--------|---------|----------|
| GPU ID Picking | Low | Medium | Medium | 1-2 frames | Complex scenes |
| CPU Ray-Casting | Medium-High | None | Low | Immediate | Simple scenes |
| Depth Reconstruction | Low | Low | Low | 1 frame | Depth-based |
| Grid-Based | Very Low | None | None | Immediate | Regular grids |

## Testing Recommendations

1. **Visual Feedback:** Add visual highlighting of picked cells
2. **Log Output:** Print grid coordinates and cell IDs
3. **Edge Cases:** Test at grid boundaries
4. **Performance:** Monitor picking overhead in profiler
5. **Accuracy:** Verify correct cell selection at various zoom levels

## Future Enhancements

- Multi-selection with drag rectangles
- Hierarchical spatial structures (BVH) for faster ray-casting
- GPU compute shader for parallel ray-cell tests
- Hover highlighting with delayed picking
- Selection history and undo/redo support
