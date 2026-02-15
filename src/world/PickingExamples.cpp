// Example implementations for different render picking methods
// This file demonstrates how to use each picking approach described in
// docs/RENDER_PICKING_IMPLEMENTATIONS.md

#include "world/CellPicking.h"
#include "world/World.h"
#include "core/Log.h"
#include <cfloat>

namespace PickingExamples {

// ============================================================================
// Example 1: Grid-Based Picking (Currently Integrated)
// ============================================================================
// This is the simplest and fastest method for the GENERATIONS grid
// Already integrated in World::pick_cell_at_screen_position()
//
// Usage:
//   CellPicking::GridPickResult result = world.pick_cell_at_screen_position(
//       mouseX, mouseY, screenWidth, screenHeight);
//   if (result.hit) {
//       world.highlight_cell(result.cellIndex);
//   }

// ============================================================================
// Example 2: CPU Ray-Casting with AABB Intersection
// ============================================================================
// This method is useful when you need more control or have irregular geometry

int find_clicked_cell_raycasting(float mouseX, float mouseY,
                                  const glm::mat4 &view,
                                  const glm::mat4 &projection,
                                  const World &world,
                                  int screenWidth,
                                  int screenHeight) {
    // Create ray from screen position
    CellPicking::Ray ray = CellPicking::screen_to_world_ray(
        mouseX, mouseY, view, projection, screenWidth, screenHeight);

    int closestCell = -1;
    float closestDist = FLT_MAX;
    const float cellSize = world.get_ubo().cell_size;

    // Test all cells for intersection
    for (size_t i = 0; i < world.get_grid().cells.size(); ++i) {
        glm::vec3 cellPos = glm::vec3(world.get_grid().cells[i].instance_position);
        
        // Create AABB for cell
        glm::vec3 aabbMin = cellPos - glm::vec3(cellSize * 0.5f);
        glm::vec3 aabbMax = cellPos + glm::vec3(cellSize * 0.5f);

        float t;
        if (CellPicking::ray_aabb_intersection(ray, aabbMin, aabbMax, t)) {
            if (t < closestDist) {
                closestDist = t;
                closestCell = static_cast<int>(i);
            }
        }
    }

    if (closestCell >= 0) {
        Log::text("{ PICK-RAY }", "Ray-casting found cell:", closestCell, "at distance:", closestDist);
    }

    return closestCell;
}

// ============================================================================
// Example 3: World Position to Grid Cell
// ============================================================================
// Useful when you already have a world position (e.g., from depth buffer)

int world_position_to_cell_index(const glm::vec3 &worldPos, const World &world) {
    const float cellSize = world.get_ubo().cell_size;
    const int gridWidth = world.get_ubo().grid_xy.x;
    const int gridHeight = world.get_ubo().grid_xy.y;

    // Convert world position to grid coordinates
    int gridX = static_cast<int>((worldPos.x / cellSize) + gridWidth / 2.0f);
    int gridZ = static_cast<int>((worldPos.z / cellSize) + gridHeight / 2.0f);

    // Check bounds
    if (gridX >= 0 && gridX < gridWidth && gridZ >= 0 && gridZ < gridHeight) {
        return gridZ * gridWidth + gridX;
    }

    return -1; // Out of bounds
}

// ============================================================================
// Example 4: Find Nearest Cell to World Position
// ============================================================================
// Alternative approach: find the cell closest to a given world position

int find_nearest_cell(const glm::vec3 &worldPos, const World &world) {
    int closestCell = -1;
    float closestDist = FLT_MAX;

    for (size_t i = 0; i < world.get_grid().cells.size(); ++i) {
        glm::vec3 cellPos = glm::vec3(world.get_grid().cells[i].instance_position);
        float dist = glm::distance(worldPos, cellPos);

        if (dist < closestDist) {
            closestDist = dist;
            closestCell = static_cast<int>(i);
        }
    }

    return closestCell;
}

// ============================================================================
// Example 5: Batch Cell Selection (Rectangle Selection)
// ============================================================================
// Select multiple cells within a screen-space rectangle

std::vector<int> select_cells_in_rectangle(float x1, float y1, float x2, float y2,
                                            const glm::mat4 &view,
                                            const glm::mat4 &projection,
                                            const World &world,
                                            int screenWidth,
                                            int screenHeight) {
    std::vector<int> selectedCells;

    // Ensure x1,y1 is top-left and x2,y2 is bottom-right
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Sample points within the rectangle
    const int samples = 20;
    for (int sx = 0; sx < samples; ++sx) {
        for (int sy = 0; sy < samples; ++sy) {
            float testX = x1 + (x2 - x1) * sx / static_cast<float>(samples);
            float testY = y1 + (y2 - y1) * sy / static_cast<float>(samples);

            CellPicking::GridPickResult result = CellPicking::pick_grid_cell(
                testX, testY, view, projection,
                world.get_ubo().grid_xy.x, world.get_ubo().grid_xy.y,
                world.get_ubo().cell_size,
                screenWidth, screenHeight, 0.0f);

            if (result.hit) {
                // Add to list if not already present
                if (std::find(selectedCells.begin(), selectedCells.end(), result.cellIndex) 
                    == selectedCells.end()) {
                    selectedCells.push_back(result.cellIndex);
                }
            }
        }
    }

    Log::text("{ PICK-RECT }", "Selected", selectedCells.size(), "cells in rectangle");
    return selectedCells;
}

// ============================================================================
// Example 6: Hover Detection with Tolerance
// ============================================================================
// Detect when mouse hovers over a cell, useful for previewing

struct HoverState {
    int lastHoveredCell = -1;
    float hoverStartTime = 0.0f;
    static constexpr float HOVER_DELAY = 0.3f; // seconds
};

int detect_hover_cell(float mouseX, float mouseY,
                      const glm::mat4 &view,
                      const glm::mat4 &projection,
                      const World &world,
                      int screenWidth,
                      int screenHeight,
                      HoverState &state,
                      float currentTime) {
    CellPicking::GridPickResult result = CellPicking::pick_grid_cell(
        mouseX, mouseY, view, projection,
        world.get_ubo().grid_xy.x, world.get_ubo().grid_xy.y,
        world.get_ubo().cell_size,
        screenWidth, screenHeight, 0.0f);

    int hoveredCell = result.hit ? result.cellIndex : -1;

    if (hoveredCell != state.lastHoveredCell) {
        // Mouse moved to different cell
        state.lastHoveredCell = hoveredCell;
        state.hoverStartTime = currentTime;
        return -1; // Not hovered long enough yet
    }

    // Check if hover delay elapsed
    if (hoveredCell >= 0 && (currentTime - state.hoverStartTime) >= HoverState::HOVER_DELAY) {
        return hoveredCell;
    }

    return -1;
}

// ============================================================================
// Example 7: Advanced Color Manipulation
// ============================================================================
// NOTE: These examples use static state for demonstration purposes.
// In production code, consider managing state differently (e.g., class members)

void cycle_cell_colors(World &world, int cellIndex) {
    if (cellIndex < 0 || cellIndex >= static_cast<int>(world.get_grid().cells.size())) {
        return;
    }

    static std::vector<glm::vec4> colorPalette = {
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green
        glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow
        glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),  // Magenta
        glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),  // Cyan
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),  // White
    };

    static std::unordered_map<int, int> cellColorIndices;
    
    int &colorIndex = cellColorIndices[cellIndex];
    colorIndex = (colorIndex + 1) % colorPalette.size();
    
    world.set_cell_color(cellIndex, colorPalette[colorIndex]);
    Log::text("{ COLOR }", "Cell", cellIndex, "color cycled to index", colorIndex);
}

void paint_cells_in_radius(World &world, int centerCellIndex, float radius, 
                           const glm::vec4 &color) {
    if (centerCellIndex < 0 || centerCellIndex >= static_cast<int>(world.get_grid().cells.size())) {
        return;
    }

    glm::vec3 centerPos = glm::vec3(world.get_grid().cells[centerCellIndex].instance_position);
    int paintedCount = 0;

    for (size_t i = 0; i < world.get_grid().cells.size(); ++i) {
        glm::vec3 cellPos = glm::vec3(world.get_grid().cells[i].instance_position);
        float dist = glm::distance(centerPos, cellPos);

        if (dist <= radius) {
            world.set_cell_color(static_cast<int>(i), color);
            paintedCount++;
        }
    }

    Log::text("{ PAINT }", "Painted", paintedCount, "cells within radius", radius);
}

} // namespace PickingExamples
