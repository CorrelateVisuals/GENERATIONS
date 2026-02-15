#!/usr/bin/env python3
"""Analyze screenshot to find cells on water-colored terrain."""
import math, glob, os, sys
from PIL import Image

files = sorted(glob.glob("screenshot/screenshot_*.png"))
if not files:
    print("No screenshots found"); sys.exit(1)
path = files[-1]
print(f"Analyzing: {os.path.basename(path)}")

img = Image.open(path).convert("RGB")
pix = img.load()
w, h = img.size

# Find white cell pixel locations
cell_pixels = []
for y in range(0, h, 2):
    for x in range(0, w, 2):
        r, g, b = pix[x, y]
        if r > 200 and g > 200 and b > 200:
            cell_pixels.append((x, y))

print(f"White cell pixel samples: {len(cell_pixels)}")

water_cells = []
land_cells = []
checked_blocks = set()

for cx, cy in cell_pixels:
    bk = (cx // 8, cy // 8)
    if bk in checked_blocks:
        continue
    checked_blocks.add(bk)

    terrain_colors = []
    for angle_step in range(8):
        angle = angle_step * math.pi / 4
        for radius in [15, 18, 22]:
            sx = int(cx + radius * math.cos(angle))
            sy = int(cy + radius * math.sin(angle))
            if 0 <= sx < w and 0 <= sy < h:
                r, g, b = pix[sx, sy]
                if not (r > 180 and g > 180 and b > 180):
                    terrain_colors.append((r, g, b))

    if not terrain_colors:
        continue

    avg_r = sum(c[0] for c in terrain_colors) / len(terrain_colors)
    avg_g = sum(c[1] for c in terrain_colors) / len(terrain_colors)
    avg_b = sum(c[2] for c in terrain_colors) / len(terrain_colors)

    is_water = (avg_b > avg_r + 5) and (avg_r < 130) and (avg_g < 140)

    if is_water:
        water_cells.append((cx, cy, int(avg_r), int(avg_g), int(avg_b)))
    else:
        land_cells.append((cx, cy, int(avg_r), int(avg_g), int(avg_b)))

print(f"\nCells on WATER terrain: {len(water_cells)}")
print(f"Cells on LAND terrain:  {len(land_cells)}")

if water_cells:
    print(f"\nSample water-cell terrain colors (R,G,B):")
    for item in water_cells[:20]:
        print(f"  pixel({item[0]},{item[1]}): terrain avg = ({item[2]},{item[3]},{item[4]})")

if land_cells:
    print(f"\nSample land-cell terrain colors (R,G,B):")
    for item in land_cells[:10]:
        print(f"  pixel({item[0]},{item[1]}): terrain avg = ({item[2]},{item[3]},{item[4]})")
