from PIL import Image
import numpy as np
from pathlib import Path
import colorsys


def analyze(path: Path) -> None:
    arr = np.array(Image.open(path).convert("RGB")).astype(np.float32) / 255.0
    flat = arr.reshape(-1, 3)

    luma = 0.2126 * flat[:, 0] + 0.7152 * flat[:, 1] + 0.0722 * flat[:, 2]
    maxc = flat.max(axis=1)
    minc = flat.min(axis=1)
    sat = np.where(maxc > 1e-6, (maxc - minc) / maxc, 0.0)

    sample = flat[::500]
    hsv = np.array([
        colorsys.rgb_to_hsv(float(px[0]), float(px[1]), float(px[2])) for px in sample
    ])
    hue = hsv[:, 0]
    hue_sat = hsv[:, 1]
    hue = hue[hue_sat > 0.1]
    if hue.size:
        bins = np.histogram(hue, bins=12, range=(0.0, 1.0))[0]
        pbin = bins / (bins.sum() + 1e-9)
        entropy = float(-(pbin[pbin > 0] * np.log2(pbin[pbin > 0])).sum())
    else:
        entropy = 0.0

    q = (flat * 15).astype(np.int16)
    quantized_color_count = int(np.unique(q, axis=0).shape[0])

    luma2 = 0.2126 * arr[:, :, 0] + 0.7152 * arr[:, :, 1] + 0.0722 * arr[:, :, 2]
    gx = np.abs(np.diff(luma2, axis=1))
    gy = np.abs(np.diff(luma2, axis=0))
    edge_density = float(((gx > 0.08).mean() + (gy > 0.08).mean()) * 0.5)

    blue_dominance = float((flat[:, 2] > flat[:, 1] + 0.08).mean())

    print(path.name)
    print("  meanRGB:", [round(float(v * 255.0), 1) for v in flat.mean(axis=0)])
    print("  luma mean/std:", round(float(luma.mean() * 255.0), 1), round(float(luma.std() * 255.0), 1))
    print("  sat mean/p90:", round(float(sat.mean()), 3), round(float(np.quantile(sat, 0.9)), 3))
    print("  hue entropy:", round(entropy, 3), "quantized colors:", quantized_color_count)
    print("  edge density:", round(edge_density, 5), "blue dominance:", round(blue_dominance, 5))


def main() -> None:
    files = sorted(Path("screenshot").glob("*.png"), key=lambda p: p.stat().st_mtime)
    if not files:
        print("No screenshots found.")
        return

    selected = files[-6:]
    print("Analyzing", len(selected), "latest screenshots")
    for file_path in selected:
        analyze(file_path)


if __name__ == "__main__":
    main()
