#!/usr/bin/env python3

from __future__ import annotations

import sys


SHADER_NODES: list[tuple[str, str, str]] = [
    ("CellsVert", "Cells", "vert"),
    ("CellsFrag", "Cells", "frag"),
    ("EngineComp", "Engine", "comp"),
    ("LandscapeVert", "Landscape", "vert"),
    ("LandscapeFrag", "Landscape", "frag"),
    ("LandscapeWireFrameTesc", "LandscapeWireFrame", "tesc"),
    ("LandscapeWireFrameTese", "LandscapeWireFrame", "tese"),
    ("PostFXComp", "PostFX", "comp"),
    ("TextureVert", "Texture", "vert"),
    ("TextureFrag", "Texture", "frag"),
    ("WaterVert", "Water", "vert"),
    ("WaterFrag", "Water", "frag"),
]

EDGES: list[tuple[str, str]] = [
    ("CellsVert", "CellsFrag"),
    ("CellsFrag", "EngineComp"),
    ("LandscapeVert", "LandscapeWireFrameTesc"),
    ("LandscapeWireFrameTesc", "LandscapeWireFrameTese"),
    ("LandscapeVert", "LandscapeFrag"),
    ("TextureVert", "TextureFrag"),
    ("WaterVert", "WaterFrag"),
    ("EngineComp", "PostFXComp"),
]

INPUT = ("CellsVert", "input_data")
OUTPUT = ("WaterFrag", "result")

DRAW_BINDINGS: list[tuple[str, str]] = [
    ("Cells", "cells_instanced"),
    ("Landscape", "grid_indexed"),
    ("LandscapeWireFrame", "grid_wireframe"),
    ("Water", "rectangle_indexed"),
    ("Texture", "rectangle_indexed"),
]


def emit_graph() -> None:
    for node_id, shader_name, extension in SHADER_NODES:
        print(f"NODE {node_id} {shader_name} {extension}")

    for source, target in EDGES:
        print(f"EDGE {source} {target}")

    for pipeline_name, draw_op in DRAW_BINDINGS:
        print(f"DRAW {pipeline_name} {draw_op}")

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
