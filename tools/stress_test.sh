#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -x "./bin/CapitalEngine" ]; then
  echo "Binary not found: ./bin/CapitalEngine"
  echo "Build first: cmake --build build --parallel \"\$(nproc)\""
  exit 1
fi

DURATION_SECONDS="${1:-180}"
GPU_FREQ_MS="${GPU_FREQ_MS:-250}"
RUN_COMPUTE="${RUN_COMPUTE:-1}"
RUN_FULL="${RUN_FULL:-1}"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
FULL_LOG="/tmp/ce_stress_full_${TIMESTAMP}.log"
COMPUTE_LOG="/tmp/ce_stress_compute_${TIMESTAMP}.log"

run_case() {
  local name="$1"
  local workload_preset="$2"
  local log_path="$3"

  echo "[stress] running ${name} (${DURATION_SECONDS}s)"

  if [ "$workload_preset" = "default" ]; then
    CE_GPU_LOG=on \
    CE_GPU_LOG_PERIODIC=on \
    CE_GPU_LOG_DETAILS=detailed \
    CE_GPU_LOG_FREQ_MS="$GPU_FREQ_MS" \
    timeout --signal=INT --kill-after=3s "${DURATION_SECONDS}s" ./bin/CapitalEngine >"$log_path" 2>&1 || true
  else
    CE_WORKLOAD_PRESET="$workload_preset" \
    CE_GPU_LOG=on \
    CE_GPU_LOG_PERIODIC=on \
    CE_GPU_LOG_DETAILS=detailed \
    CE_GPU_LOG_FREQ_MS="$GPU_FREQ_MS" \
    timeout --signal=INT --kill-after=3s "${DURATION_SECONDS}s" ./bin/CapitalEngine >"$log_path" 2>&1 || true
  fi

  echo "[stress] ${name} log: ${log_path}"
  echo "[stress] ${name} summary:"
  grep -nE 'Validation (Error|Performance Warning)|VUID|!ERROR!|Segmentation fault|abort|exception|failed' "$log_path" | head -n 40 || true
  echo "--- memory samples ---"
  grep -nE '\{ GPU \} runtime memory' "$log_path" | head -n 5 || true
  grep -nE '\{ GPU \} runtime memory' "$log_path" | tail -n 5 || true
  echo "--- shutdown tail ---"
  tail -n 12 "$log_path"
  echo
}

if [ "$RUN_FULL" = "1" ]; then
  run_case "full" "default" "$FULL_LOG"
fi

if [ "$RUN_COMPUTE" = "1" ]; then
  run_case "compute-only" "compute_only" "$COMPUTE_LOG"
fi

echo "[stress] done"
