#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

while true; do
  ./assets/tools/daily_report.sh
  # Sleep for 24 hours
  sleep 86400
 done
