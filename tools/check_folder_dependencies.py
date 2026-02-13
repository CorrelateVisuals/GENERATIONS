#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
SOURCE_EXTENSIONS = {".h", ".hpp", ".cpp", ".cxx", ".cc"}


@dataclass(frozen=True)
class Violation:
    source_file: Path
    source_folder: str
    target_folder: str
    include_text: str
    line_number: int


def source_files(src_root: Path) -> list[Path]:
    return [
        path
        for path in src_root.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_EXTENSIONS
    ]


def build_name_index(files: list[Path], src_root: Path) -> dict[str, set[str]]:
    name_to_folders: dict[str, set[str]] = {}
    for file_path in files:
        try:
            folder = file_path.relative_to(src_root).parts[0]
        except (ValueError, IndexError):
            continue
        name_to_folders.setdefault(file_path.name, set()).add(folder)
    return name_to_folders


def parse_local_includes(path: Path) -> list[tuple[int, str]]:
    includes: list[tuple[int, str]] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
        match = INCLUDE_PATTERN.match(line)
        if match:
            includes.append((line_number, match.group(1).strip()))
    return includes


def resolve_target_folder(
    source_file: Path,
    include_text: str,
    src_root: Path,
    known_folders: set[str],
    name_index: dict[str, set[str]],
) -> str | None:
    include_path = Path(include_text)
    include_parts = include_path.parts
    if include_parts and include_parts[0] in known_folders:
        return include_parts[0]

    if include_text.startswith("./") or include_text.startswith("../"):
        resolved = (source_file.parent / include_path).resolve()
        try:
            return resolved.relative_to(src_root).parts[0]
        except (ValueError, IndexError):
            return None

    candidate_folders = name_index.get(include_path.name, set())
    if len(candidate_folders) == 1:
        return next(iter(candidate_folders))

    return None


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    src_root = repo_root / "src"

    if not src_root.exists():
        print(f"ERROR: source root not found: {src_root}")
        return 2

    files = source_files(src_root)
    known_folders = {
        path.name
        for path in src_root.iterdir()
        if path.is_dir()
    }

    forbidden_targets: dict[str, set[str]] = {
        "render": {"app"},
        "world": {"app", "render"},
        "platform": {"app", "world", "render"},
        "io": {"app", "world", "render"},
    }

    name_index = build_name_index(files, src_root)
    violations: list[Violation] = []

    for file_path in files:
        try:
            source_folder = file_path.relative_to(src_root).parts[0]
        except (ValueError, IndexError):
            continue

        blocked = forbidden_targets.get(source_folder)
        if not blocked:
            continue

        for line_number, include_text in parse_local_includes(file_path):
            target_folder = resolve_target_folder(
                source_file=file_path,
                include_text=include_text,
                src_root=src_root,
                known_folders=known_folders,
                name_index=name_index,
            )
            if target_folder in blocked:
                violations.append(
                    Violation(
                        source_file=file_path,
                        source_folder=source_folder,
                        target_folder=target_folder,
                        include_text=include_text,
                        line_number=line_number,
                    )
                )

    if violations:
        print("Folder dependency check failed:")
        for violation in violations:
            rel_source = violation.source_file.relative_to(repo_root)
            print(
                f"- {rel_source}:{violation.line_number} "
                f"{violation.source_folder} -> {violation.target_folder} via \"{violation.include_text}\""
            )
        return 1

    print("Folder dependency check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
