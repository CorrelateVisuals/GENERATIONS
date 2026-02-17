#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

if [ ! -x "./out/bin/CapitalEngine" ]; then
  cmake --preset dev
  cmake --build --preset dev
fi

exec ./out/bin/CapitalEngine "$@"
