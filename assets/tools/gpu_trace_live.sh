#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

if [ ! -x "./out/bin/CapitalEngine" ]; then
	echo "Binary not found: ./out/bin/CapitalEngine"
	echo "Build first: cmake --preset dev && cmake --build --preset dev"
	exit 1
fi

: > log.txt

cleanup() {
	set +e
	if [ -n "${TAIL_PID:-}" ]; then
		kill "$TAIL_PID" >/dev/null 2>&1 || true
	fi
	if [ -n "${APP_PID:-}" ]; then
		kill "$APP_PID" >/dev/null 2>&1 || true
	fi
}
trap cleanup EXIT INT TERM

echo "[trace] starting CapitalEngine with CE_GPU_TRACE=1"
CE_GPU_TRACE=1 ./out/bin/CapitalEngine &
APP_PID=$!

echo "[trace] streaming GPU flow from log.txt"
echo "[trace] categories: mdl/map/wr/xfr/que/lck/dst/mem/sync"

tail -n +1 -F log.txt | \
	stdbuf -oL grep --line-buffered -E "\{ (mdl|MAP|WR|XFR|QUE|LCK|DST|GPU|SWP|MEM|SYNC|cmd|img|!!!|PERF|REP|1\.\.|\.\.1) \}|Model|copy|alloc|submit|wait|destroy|transition|queue|fence|map|unmap|write|selected|Loaded" &
TAIL_PID=$!

wait "$APP_PID"
