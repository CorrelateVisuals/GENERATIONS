#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -x "./bin/CapitalEngine" ]; then
	echo "Binary not found: ./bin/CapitalEngine"
	echo "Build first: cmake --build build --parallel \"\$(nproc)\""
	exit 1
fi

# Default to 5 hours if not specified
DURATION_SECONDS="${1:-18000}"

echo "========================================"
echo "GENERATIONS Deep Test - Memory Leak Detection"
echo "========================================"
echo "Duration: $DURATION_SECONDS seconds ($((DURATION_SECONDS / 3600)) hours)"
echo "Memory reports will be logged every hour"
echo "Log file: log.txt"
echo "========================================"
echo ""

# Clear previous log
: > log.txt

cleanup() {
	set +e
	if [ -n "${APP_PID:-}" ]; then
		kill "$APP_PID" >/dev/null 2>&1 || true
	fi
}
trap cleanup EXIT INT TERM

echo "Starting deep test..."
# Note: Application writes directly to log.txt via Log::log_file
CE_DEEPTEST_DURATION="$DURATION_SECONDS" ./bin/CapitalEngine &
APP_PID=$!

echo "Application started (PID: $APP_PID)"
echo "Monitoring for completion..."
echo ""

# Wait for the process to complete
wait "$APP_PID"
EXIT_CODE=$?

echo ""
echo "========================================"
echo "Deep test completed with exit code: $EXIT_CODE"
echo "========================================"
echo ""
echo "Extracting memory reports from log.txt:"
echo ""

# Show all memory reports from the log
grep -E "DEEP TEST MEMORY REPORT|Runtime:|Vulkan" log.txt || echo "No memory reports found"

echo ""
echo "Full log available in: log.txt"
echo "========================================"

exit $EXIT_CODE
