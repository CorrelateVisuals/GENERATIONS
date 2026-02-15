// Standalone test program to demonstrate cell picking logic
// This can be compiled and run independently to verify the picking algorithms

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

// Minimal GLM-like structs for demonstration
struct vec2 { float x, y; };
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };
struct mat4 { float m[16]; }; // Simplified for demo

namespace TestPicking {

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct GridPickResult {
    bool hit;
    int cellX;
    int cellY;
    int cellIndex;
    vec3 worldPosition;
};

// Simplified picking for demonstration
GridPickResult pick_grid_cell_demo(float mouseX, float mouseY,
                                    int gridWidth, int gridHeight,
                                    float cellSize,
                                    int screenWidth, int screenHeight) {
    GridPickResult result;
    result.hit = false;
    
    // Simple orthographic projection for demo
    // Convert screen to world coordinates
    float worldX = (mouseX - screenWidth / 2.0f) * cellSize / 50.0f;
    float worldZ = (mouseY - screenHeight / 2.0f) * cellSize / 50.0f;
    
    // Convert to grid coordinates
    int gridX = static_cast<int>((worldX / cellSize) + gridWidth / 2.0f);
    int gridZ = static_cast<int>((worldZ / cellSize) + gridHeight / 2.0f);
    
    // Check bounds
    result.hit = (gridX >= 0 && gridX < gridWidth && 
                  gridZ >= 0 && gridZ < gridHeight);
    
    if (result.hit) {
        result.cellX = gridX;
        result.cellY = gridZ;
        result.cellIndex = gridZ * gridWidth + gridX;
        result.worldPosition = {worldX, 0.0f, worldZ};
    }
    
    return result;
}

void test_picking() {
    std::cout << "=== Render Picking Test ===\n\n";
    
    const int gridWidth = 10;
    const int gridHeight = 10;
    const float cellSize = 1.0f;
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    // Test various click positions
    struct TestCase {
        float mouseX, mouseY;
        const char* description;
    };
    
    std::vector<TestCase> testCases = {
        {400, 300, "Center of screen"},
        {0, 0, "Top-left corner"},
        {800, 600, "Bottom-right corner"},
        {200, 150, "Upper-left quadrant"},
        {600, 450, "Lower-right quadrant"}
    };
    
    std::cout << "Grid: " << gridWidth << "x" << gridHeight 
              << ", Cell size: " << cellSize << "\n";
    std::cout << "Screen: " << screenWidth << "x" << screenHeight << "\n\n";
    
    for (const auto& test : testCases) {
        GridPickResult result = pick_grid_cell_demo(
            test.mouseX, test.mouseY,
            gridWidth, gridHeight, cellSize,
            screenWidth, screenHeight);
        
        std::cout << "Click at (" << test.mouseX << ", " << test.mouseY 
                  << ") - " << test.description << ":\n";
        
        if (result.hit) {
            std::cout << "  ✓ HIT - Grid[" << result.cellX << ", " << result.cellY << "]"
                      << " Index: " << result.cellIndex
                      << " World: (" << result.worldPosition.x << ", " 
                      << result.worldPosition.y << ", " 
                      << result.worldPosition.z << ")\n";
        } else {
            std::cout << "  ✗ MISS - Outside grid bounds\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "=== Test Complete ===\n";
}

} // namespace TestPicking

int main() {
    TestPicking::test_picking();
    
    std::cout << "\nThis demonstrates the picking logic used in GENERATIONS.\n";
    std::cout << "See docs/RENDER_PICKING_IMPLEMENTATIONS.md for full details.\n";
    
    return 0;
}
