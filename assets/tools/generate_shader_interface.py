#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import sys


@dataclass
class Field:
	schema_type: str
	cxx_name: str
	glsl_name: str
	default_values: list[str]


TYPE_TO_CPP = {
	"vec4": "glm::vec4",
	"ivec2": "glm::ivec2",
	"float": "float",
	"mat4": "glm::mat4",
}


def find_repo_root(script_path: Path) -> Path:
	for candidate in script_path.parents:
		if (candidate / "CMakeLists.txt").exists() and (candidate / "src").exists():
			return candidate
	raise RuntimeError(f"repository root not found from script path: {script_path}")


def parse_schema(schema_path: Path) -> list[Field]:
	fields: list[Field] = []
	for raw in schema_path.read_text(encoding="utf-8").splitlines():
		line = raw.strip()
		if not line or line.startswith("#"):
			continue

		default_values: list[str] = []
		left = line
		if "=" in line:
			left, right = line.split("=", 1)
			default_values = right.strip().split()

		parts = left.split()
		if len(parts) < 3:
			raise RuntimeError(f"invalid schema line: {raw}")

		schema_type, cxx_name, glsl_name = parts[0], parts[1], parts[2]
		if schema_type not in TYPE_TO_CPP:
			raise RuntimeError(f"unsupported schema type '{schema_type}' in line: {raw}")

		fields.append(Field(schema_type, cxx_name, glsl_name, default_values))
	return fields


def cpp_default(field: Field) -> str:
	if field.default_values:
		if field.schema_type == "vec4":
			vals = ", ".join(f"{v}f" for v in field.default_values)
			return f"{{{vals}}}"
		if field.schema_type == "ivec2":
			vals = ", ".join(field.default_values)
			return f"{{{vals}}}"
		if field.schema_type == "float":
			return f"{{{field.default_values[0]}f}}"
		if field.schema_type == "mat4":
			return "{}"

	if field.schema_type == "float":
		return "{0.0f}"
	return "{}"


def generate_cpp(fields: list[Field]) -> str:
	lines = [
		"#pragma once",
		"",
		"#include <glm/glm.hpp>",
		"",
		"namespace CE::ShaderInterface {",
		"",
		"struct alignas(16) ParameterUBO {",
	]

	for field in fields:
		cpp_type = TYPE_TO_CPP[field.schema_type]
		lines.append(f"  {cpp_type} {field.cxx_name}{cpp_default(field)};")

	lines.extend([
		"};",
		"",
		"} // namespace CE::ShaderInterface",
		"",
	])
	return "\n".join(lines)


def generate_glsl(fields: list[Field]) -> str:
	lines = [
		"#ifndef PARAMETER_UBO_GLSL",
		"#define PARAMETER_UBO_GLSL",
		"",
		"#extension GL_GOOGLE_include_directive : enable",
		"",
		"// Generated from src/vulkan_resources/ParameterUBO.schema. Do not edit by hand.",
		"#ifndef UBO_LIGHT_NAME",
		"#define UBO_LIGHT_NAME light",
		"#endif",
		"",
		"layout (binding = 0) uniform ParameterUBO {",
	]

	for field in fields:
		glsl_name = field.glsl_name
		if glsl_name == "light":
			glsl_name = "UBO_LIGHT_NAME"
		lines.append(f"    {field.schema_type} {glsl_name};")

	lines.extend([
		"} ubo;",
		"",
		"#endif",
		"",
	])
	return "\n".join(lines)


def main() -> int:
	script = Path(__file__).resolve()
	repo_root = find_repo_root(script)

	schema_path = repo_root / "src" / "vulkan_resources" / "ParameterUBO.schema"
	cpp_out = repo_root / "src" / "vulkan_pipelines" / "ShaderInterface.h"
	glsl_out = repo_root / "shaders" / "ParameterUBO.glsl"

	fields = parse_schema(schema_path)

	cpp_out.parent.mkdir(parents=True, exist_ok=True)
	glsl_out.parent.mkdir(parents=True, exist_ok=True)

	cpp_out.write_text(generate_cpp(fields), encoding="utf-8")
	glsl_out.write_text(generate_glsl(fields), encoding="utf-8")

	print(f"Generated: {cpp_out}")
	print(f"Generated: {glsl_out}")
	return 0


if __name__ == "__main__":
	sys.exit(main())
