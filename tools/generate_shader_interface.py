#!/usr/bin/env python3
"""Generate C++ and GLSL shader interface from a single schema file."""
from __future__ import annotations

from pathlib import Path
from typing import List, Tuple

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "src" / "core" / "ParameterUBO.schema"
CPP_OUT = ROOT / "src" / "core" / "ShaderInterface.h"
GLSL_OUT = ROOT / "shaders" / "ParameterUBO.glsl"

TYPE_MAP = {
    "vec4": "glm::vec4",
    "ivec2": "glm::ivec2",
    "mat4": "glm::mat4",
    "float": "float",
    "int": "int",
}

ALIGN_TYPES = {"mat4"}


def parse_schema(lines: List[str]) -> List[Tuple[str, str, str, List[str]]]:
    fields: List[Tuple[str, str, str, List[str]]] = []
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "//" in line:
            line = line.split("//", 1)[0].strip()
        if not line:
            continue
        if "=" in line:
            left, right = line.split("=", 1)
            left = left.strip()
            defaults = right.strip().split()
        else:
            left = line
            defaults = []
        parts = left.split()
        if len(parts) != 3:
            raise ValueError(f"Invalid schema line: {raw}")
        ftype, cxx_name, glsl_name = parts
        fields.append((ftype, cxx_name, glsl_name, defaults))
    return fields


def fmt_default(ftype: str, defaults: List[str]) -> str:
    if not defaults:
        return "{}"
    if ftype == "vec4" and len(defaults) == 4:
        vals = ", ".join(f"{v}f" for v in defaults)
        return f"{{{vals}}}"
    if ftype == "float" and len(defaults) == 1:
        return f"{{{defaults[0]}f}}"
    return "{}"


def write_cpp(fields: List[Tuple[str, str, str, List[str]]]) -> None:
    lines: List[str] = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <glm/glm.hpp>")
    lines.append("")
    lines.append("namespace CE::ShaderInterface {")
    lines.append("")
    lines.append("// Generated from src/core/ParameterUBO.schema. Do not edit by hand.")
    lines.append("struct ParameterUBO {")
    for ftype, cxx_name, _glsl_name, defaults in fields:
        cxx_type = TYPE_MAP.get(ftype)
        if not cxx_type:
            raise ValueError(f"Unsupported type: {ftype}")
        align = "alignas(16) " if ftype in ALIGN_TYPES else ""
        default_str = fmt_default(ftype, defaults)
        if default_str != "{}":
            lines.append(f"  {align}{cxx_type} {cxx_name}{default_str};")
        else:
            lines.append(f"  {align}{cxx_type} {cxx_name}{{}};")
    # Add constructor if expected fields exist.
    names = {cxx_name for _t, cxx_name, _g, _d in fields}
    if {"light", "grid_xy", "water_threshold", "cell_size", "water_rules"}.issubset(names):
        lines.append("")
        lines.append("  ParameterUBO(glm::vec4 l, glm::ivec2 xy, float w, float s, glm::vec4 rules)")
        lines.append("      : light(l), grid_xy(xy), water_threshold(w), cell_size(s), water_rules(rules) {}")
    lines.append("};")
    lines.append("")
    lines.append("} // namespace CE::ShaderInterface")
    lines.append("")
    CPP_OUT.write_text("\n".join(lines), encoding="utf-8")


def write_glsl(fields: List[Tuple[str, str, str, List[str]]]) -> None:
    lines: List[str] = []
    lines.append("#ifndef PARAMETER_UBO_GLSL")
    lines.append("#define PARAMETER_UBO_GLSL")
    lines.append("")
    lines.append("#extension GL_GOOGLE_include_directive : enable")
    lines.append("")
    lines.append("// Generated from src/core/ParameterUBO.schema. Do not edit by hand.")
    lines.append("#ifndef UBO_LIGHT_NAME")
    lines.append("#define UBO_LIGHT_NAME light")
    lines.append("#endif")
    lines.append("")
    lines.append("layout (binding = 0) uniform ParameterUBO {")
    for ftype, _cxx_name, glsl_name, _defaults in fields:
        name = "UBO_LIGHT_NAME" if glsl_name == "light" else glsl_name
        lines.append(f"    {ftype} {name};")
    lines.append("} ubo;")
    lines.append("")
    lines.append("#endif")
    lines.append("")
    GLSL_OUT.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    fields = parse_schema(SCHEMA.read_text(encoding="utf-8").splitlines())
    write_cpp(fields)
    write_glsl(fields)


if __name__ == "__main__":
    main()
