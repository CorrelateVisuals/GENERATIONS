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
    ("Cells", "instanced:cells"),
    ("Landscape", "indexed:grid"),
    ("LandscapeWireFrame", "indexed:grid"),
    ("Water", "indexed:rectangle"),
    ("Texture", "indexed:rectangle"),
]

PIPELINES: list[tuple[str, str, str, str | None]] = [
    ("Cells", "graphics", "CellsVert,CellsFrag", None),
    ("Engine", "compute", "EngineComp", "0,0,0"),
    ("Landscape", "graphics", "LandscapeVert,LandscapeFrag", None),
    ("LandscapeWireFrame",
     "graphics",
     "LandscapeVert,LandscapeWireFrameTesc,LandscapeWireFrameTese,LandscapeFrag",
     None),
    ("Texture", "graphics", "TextureVert,TextureFrag", None),
    ("Water", "graphics", "WaterVert,WaterFrag", None),
    ("PostFX", "compute", "PostFXComp", "0,0,0"),
]

SETTINGS: list[tuple[str, str]] = [
    ("world.timer_speed", "25.0"),
    ("world.water_threshold", "0.1"),
    ("world.light_pos", "0.0,20.0,20.0,0.0"),

    ("camera.zoom_speed", "0.15"),
    ("camera.panning_speed", "0.3"),
    ("camera.field_of_view", "40.0"),
    ("camera.near_clipping", "0.1"),
    ("camera.far_clipping", "1000.0"),
    ("camera.position", "0.0,0.0,60.0"),
    ("camera.arcball_tumble_mult", "0.9"),
    ("camera.arcball_pan_mult", "0.85"),
    ("camera.arcball_dolly_mult", "0.8"),

    ("geometry.rectangle", "0"),
    ("geometry.sphere", "2"),

    ("terrain.grid_width", "100"),
    ("terrain.grid_height", "100"),
    ("terrain.alive_cells", "2000"),
    ("terrain.cell_size", "0.5"),
    ("terrain.layer1.roughness", "0.4"),
    ("terrain.layer1.octaves", "10"),
    ("terrain.layer1.scale", "2.2"),
    ("terrain.layer1.amplitude", "10.0"),
    ("terrain.layer1.exponent", "2.0"),
    ("terrain.layer1.frequency", "2.0"),
    ("terrain.layer1.height_offset", "0.0"),
    ("terrain.layer2.roughness", "1.0"),
    ("terrain.layer2.octaves", "10"),
    ("terrain.layer2.scale", "2.2"),
    ("terrain.layer2.amplitude", "1.0"),
    ("terrain.layer2.exponent", "1.0"),
    ("terrain.layer2.frequency", "2.0"),
    ("terrain.layer2.height_offset", "0.0"),
    ("terrain.blend_factor", "0.5"),
    ("terrain.absolute_height", "0.0"),
]


def emit_graph() -> None:
    for node_id, shader_name, extension in SHADER_NODES:
        print(f"NODE {node_id} {shader_name} {extension}")

    for source, target in EDGES:
        print(f"EDGE {source} {target}")

    for pipeline_name, draw_op in DRAW_BINDINGS:
        print(f"DRAW {pipeline_name} {draw_op}")

    for pipeline_name, pipeline_type, shaders, workgroups in PIPELINES:
        if workgroups is None:
            print(f"PIPELINE {pipeline_name} {pipeline_type} {shaders}")
        else:
            print(f"PIPELINE {pipeline_name} {pipeline_type} {shaders} {workgroups}")

    for key, value in SETTINGS:
        print(f"SETTING {key} {value}")

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
