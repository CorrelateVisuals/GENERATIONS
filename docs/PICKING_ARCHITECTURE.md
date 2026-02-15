# Render Picking System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     GENERATIONS Render Picking                   │
│                         System Architecture                       │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                        USER INPUT LAYER                           │
└──────────────────────────────────────────────────────────────────┘
                               │
                         Mouse Click
                      (Screen Coordinates)
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│                   Window::set_mouse()                             │
│  • Captures mouse button state                                   │
│  • Stores click position in normalized coords [0,1]              │
│  • Logs click events                                             │
└──────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│              CapitalEngine::handle_mouse_click()                  │
│  • Detects new click events                                      │
│  • Converts normalized coords to screen pixels                   │
│  • Routes to picking system                                      │
└──────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│         World::pick_cell_at_screen_position()                     │
│  • Entry point for cell picking                                  │
│  • Provides camera matrices and grid parameters                  │
│  • Returns GridPickResult                                        │
└──────────────────────────────────────────────────────────────────┘
                               │
                               ▼
╔══════════════════════════════════════════════════════════════════╗
║                    PICKING METHOD SELECTION                       ║
╚══════════════════════════════════════════════════════════════════╝
                               │
              ┌────────────────┼────────────────┬─────────────────┐
              ▼                ▼                ▼                 ▼
    ┌─────────────┐  ┌──────────────┐  ┌─────────────┐  ┌─────────────┐
    │   Method 1  │  │   Method 2   │  │  Method 3   │  │  Method 4   │
    │  GPU ID     │  │  CPU Ray     │  │   Depth     │  │Grid-Based ✓ │
    │  Picking    │  │  Casting     │  │  Recon.     │  │ (Active)    │
    └─────────────┘  └──────────────┘  └─────────────┘  └─────────────┘
                                                                  │
                     ┌────────────────────────────────────────────┘
                     ▼
┌──────────────────────────────────────────────────────────────────┐
│          CellPicking::pick_grid_cell()                            │
│  1. screen_to_world_ray()                                        │
│     - Convert screen coords to NDC                               │
│     - Apply inverse projection & view matrices                   │
│     - Create ray from camera through mouse position              │
│                                                                   │
│  2. Ray-Plane Intersection                                       │
│     - Intersect ray with grid plane (y = 0)                      │
│     - Calculate world position of intersection                   │
│                                                                   │
│  3. World to Grid Coordinates                                    │
│     - Convert world position to grid indices                     │
│     - Check bounds                                               │
│     - Calculate linear cell index                                │
│                                                                   │
│  4. Return GridPickResult                                        │
│     - hit: bool                                                  │
│     - cellX, cellY: grid coordinates                             │
│     - cellIndex: linear array index                              │
│     - worldPosition: 3D intersection point                       │
└──────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│                    RESULT PROCESSING                              │
│  if (result.hit) {                                               │
│      Log picked cell information                                 │
│      Trigger color change                                        │
│  }                                                               │
└──────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│              World::highlight_cell(cellIndex)                     │
│  • Updates cell color in CPU-side data                           │
│  • Sets color to red (1.0, 0.0, 0.0, 1.0)                       │
│  • Color will be uploaded to GPU on next frame                   │
└──────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────┐
│                   GPU RENDERING (Next Frame)                      │
│  • Cell data uploaded to vertex buffer                           │
│  • Cells.vert shader reads cell.color                           │
│  • Fragment shader applies color                                 │
│  • Clicked cell appears RED on screen                            │
└──────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════

                     ALTERNATIVE METHODS

┌──────────────────────────────────────────────────────────────────┐
│  METHOD 1: GPU ID Picking                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌─────────────┐        │
│  │ Render Pass  │ -> │  ID Buffer   │ -> │  CPU Read   │        │
│  │ (ID Shader)  │    │ (uint texture)│    │  Pixel      │        │
│  └──────────────┘    └──────────────┘    └─────────────┘        │
│                                                                   │
│  Shaders: CellsPicking.vert, CellsPicking.frag                   │
│  Pro: Most accurate for complex geometry                         │
│  Con: Requires extra render pass + readback                      │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│  METHOD 2: CPU Ray-Casting                                        │
│  ┌──────────────┐    ┌──────────────┐    ┌─────────────┐        │
│  │ Create Ray   │ -> │ Test AABBs   │ -> │  Find       │        │
│  │ (screen->3D) │    │ (all cells)  │    │  Closest    │        │
│  └──────────────┘    └──────────────┘    └─────────────┘        │
│                                                                   │
│  Function: ray_aabb_intersection()                               │
│  Pro: No GPU resources needed                                    │
│  Con: CPU overhead for many cells                                │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│  METHOD 3: Depth Buffer Reconstruction                            │
│  ┌──────────────┐    ┌──────────────┐    ┌─────────────┐        │
│  │ Read Depth   │ -> │ Reconstruct  │ -> │  Find       │        │
│  │ at Pixel     │    │ World Pos    │    │  Nearest    │        │
│  └──────────────┘    └──────────────┘    └─────────────┘        │
│                                                                   │
│  Function: reconstruct_world_pos()                               │
│  Pro: Uses existing depth buffer                                 │
│  Con: Ambiguous for overlapping geometry                         │
└──────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════

                    DATA FLOW DIAGRAM

    Screen Space          World Space           Grid Space
    ────────────          ───────────           ──────────
                                                    
    (x, y)                                          
    pixels                                          
       │                                            
       │ normalize                                  
       ▼                                            
    (0-1, 0-1)                                      
    NDC coords                                      
       │                                            
       │ inverse projection                         
       ▼                                            
    Ray (origin,                                    
         direction)                                 
       │                                            
       │ plane intersection                         
       ▼                                            
    (wx, wy, wz)                                    
    world coords                                    
       │                                            
       │ grid conversion                            
       ▼                                            
                                               (gx, gy)
                                               grid coords
                                                    │
                                                    │
                                                    ▼
                                               cellIndex
                                               = gy * width + gx

═══════════════════════════════════════════════════════════════════

                    PERFORMANCE METRICS

    Grid-Based Picking (Method 4 - Current):
    • Latency: < 0.01 ms per click
    • Memory: None (no additional buffers)
    • Accuracy: Perfect for planar grids
    • Scalability: O(1) - constant time

    Ray-Casting (Method 2):
    • Latency: ~0.5-2 ms for 19,600 cells
    • Memory: None
    • Accuracy: Excellent for any geometry
    • Scalability: O(n) - linear with cell count

    GPU ID Picking (Method 1):
    • Latency: 1-2 frames (readback delay)
    • Memory: 1 offscreen buffer (screenWidth × screenHeight × 4 bytes)
    • Accuracy: Perfect for all cases
    • Scalability: O(1) after render

═══════════════════════════════════════════════════════════════════
