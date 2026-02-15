#!/usr/bin/env python3
"""Find all distinct object colors in the screenshot, especially cell-like bright spots."""
import glob, os, sys
from PIL import Image
from collections import Counter

files = sorted(glob.glob("/home/johannes/Documents/GitHub/GENERATIONS/screenshot/screenshot_*.png"))
if not files:
    print("No screenshots found"); sys.exit(1)
path = files[-1]
print(f"Analyzing: {os.path.basename(path)}")

img = Image.open(path).convert("RGB")
pix = img.load()
w, h = img.size
print(f"Size: {w}x{h}")

# Build a histogram of quantized colors (bucket by 32)
buckets = Counter()
bright_pixels = []
for y in range(0, h, 2):
    for x in range(0, w, 2):
        r, g, b = pix[x, y]
        bucket = ((r // 32) * 32, (g // 32) * 32, (b // 32) * 32)
        buckets[bucket] += 1
        # Collect unusually bright or saturated pixels (potential cells)
        luminance = 0.299 * r + 0.587 * g + 0.114 * b
        if luminance > 140 and max(r, g, b) > 160:
            bright_pixels.append((x, y, r, g, b))

print(f"\nTop 25 color buckets:")
for bucket, count in buckets.most_common(25):
    print(f"  RGB~{bucket}: {count} pixels")

print(f"\nBright pixel count (lum>140, max>160): {len(bright_pixels)}")
if bright_pixels:
    # Show some samples
    print("Samples:")
    import random
    samples = random.sample(bright_pixels, min(20, len(bright_pixels)))
    for x, y, r, g, b in sorted(samples):
        print(f"  ({x},{y}): RGB=({r},{g},{b})")

# Also check: are there any pixels that look like cells (greyish-white)?
cell_like = 0
for y in range(0, h, 2):
    for x in range(0, w, 2):
        r, g, b = pix[x, y]
        if r > 150 and g > 150 and b > 150 and abs(r - g) < 30 and abs(g - b) < 30:
            cell_like += 1
print(f"\nGrey/white cell-like pixels (R,G,B>150, balanced): {cell_like}")
