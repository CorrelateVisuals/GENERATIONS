#!/usr/bin/env python3

from __future__ import annotations

import sys


SHADER_PAIRS: list[tuple[str, str]] = [
    ("CellsVert", "vert"),
    ("CellsFrag", "frag"),
    ("EngineComp", "comp"),
    ("LandscapeVert", "vert"),
    ("LandscapeFrag", "frag"),
    ("LandscapeWireFrameTesc", "tesc"),
    ("LandscapeWireFrameTese", "tese"),
    ("PostFXComp", "comp"),
    ("TextureVert", "vert"),
    ("TextureFrag", "frag"),
    ("WaterVert", "vert"),
    ("WaterFrag", "frag"),
]

EDGES: list[tuple[str, str]] = [
    ("CellsVert", "CellsFrag"),
    ("CellsFrag", "EngineComp"),
    ("LandscapeWireFrameTesc", "LandscapeWireFrameTese"),
    ("LandscapeWireFrameTese", "LandscapeVert"),
    ("LandscapeVert", "LandscapeFrag"),
    ("TextureVert", "TextureFrag"),
    ("WaterVert", "WaterFrag"),
    ("EngineComp", "PostFXComp"),
]

INPUT = ("CellsVert", "input_data")
OUTPUT = ("WaterFrag", "result")


def emit_graph() -> None:
    for name, extension in SHADER_PAIRS:
        print(f"NODE {name} {name} {extension}")

    for source, target in EDGES:
        print(f"EDGE {source} {target}")

    print(f"INPUT {INPUT[0]} {INPUT[1]}")
    print(f"OUTPUT {OUTPUT[0]} {OUTPUT[1]}")


def main() -> int:
    if "--emit-graph" in sys.argv:
        emit_graph()
        return 0

    print("Usage: all_shaders_graph.py --emit-graph")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
