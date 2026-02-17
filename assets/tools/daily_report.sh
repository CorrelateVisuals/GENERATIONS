#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
LOG_DIR="$ROOT_DIR/logs"
REPORT_DATE=$(date +%Y%m%d)
REPORT_FILE="$LOG_DIR/daily_report_${REPORT_DATE}.md"

mkdir -p "$LOG_DIR"

cd "$ROOT_DIR"

echo "# Daily Repo Report - ${REPORT_DATE}" > "$REPORT_FILE"
{
  echo
  echo "## Git Summary"
  echo "- Branch: $(git rev-parse --abbrev-ref HEAD)"
  echo "- Commit: $(git rev-parse --short HEAD)"
  echo "- Divergence (main...HEAD): $(git rev-list --left-right --count main...HEAD | tr '\t' '/')"
  echo "- Dirty files: $(git status --porcelain | wc -l | tr -d ' ')"
  echo
  echo "### Changed Files vs main"
  git diff --name-only main...HEAD | sed 's/^/- /'

  echo
  echo "## Architecture Boundary Check"
  if python3 assets/tools/check_folder_dependencies.py > /tmp/arch_check.log 2>&1; then
    echo "- Status: OK"
  else
    echo "- Status: FAIL"
  fi
  echo "\n\n<details><summary>Architecture Check Output</summary>\n\n\`\`\`\n$(cat /tmp/arch_check.log)\n\`\`\`\n</details>"

  echo
  echo "## Modern C++ Heuristics"
  echo "- Raw new/delete usage (src/):"
  grep -R --line-number -E "\bnew\b|\bdelete\b" src || true
  echo
  echo "- C allocation (src/):"
  grep -R --line-number -E "\bmalloc\b|\bcalloc\b|\bfree\b|\brealloc\b" src || true
  echo
  echo "- C-style cast (src/):"
  grep -R --line-number -E "\([^\)]*\)\s*\w+" src | head -n 40 || true

  echo
  echo "## Recent Shader Interface"
  if [ -f src/vulkan_resources/ParameterUBO.schema ]; then
    echo "\n\`\`\`\n$(cat src/vulkan_resources/ParameterUBO.schema)\n\`\`\`"
  fi
} >> "$REPORT_FILE"

printf "Report written: %s\n" "$REPORT_FILE"
