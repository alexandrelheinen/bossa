#!/bin/bash
# BOSSA pre-push quality gates — run before every git push on a PR branch.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

SKIP_CROSS=0
SKIP_HARDWARE=0

usage() {
  echo "Usage: $(basename "$0") [--skip-cross] [--skip-hardware]"
  echo "  --skip-cross     Skip ARM64 cross-compilation (faster local iteration)"
  echo "  --skip-hardware  Document only; hardware deploy is always manual"
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-cross) SKIP_CROSS=1 ;;
    --skip-hardware) SKIP_HARDWARE=1 ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
  shift
done

cd "${PROJECT_DIR}"

echo "=== Formatting ==="
"${SCRIPT_DIR}/formatting.sh"

echo "=== Native build (x86_64) ==="
./scripts/build.sh

echo "=== Unit tests ==="
if [[ -f build/CTestTestfile.cmake ]]; then
  (cd build && ctest --output-on-failure -V)
else
  echo "No CTest configuration found (tests/ not yet populated). Skipping."
fi

if [[ "${SKIP_CROSS}" -eq 0 ]]; then
  echo "=== Cross-compile (ARM64 / Raspberry Pi 5) ==="
  ./scripts/build.sh -t toolchain-arm64.cmake
else
  echo "=== Cross-compile skipped (--skip-cross) ==="
fi

if [[ "${SKIP_HARDWARE}" -eq 0 ]]; then
  echo "=== Hardware smoke test ==="
  echo "Manual step: deploy to Pi 5 when the change touches drivers, I/O, or timing."
  echo "  ./scripts/sync.sh -t pi@raspberry.local"
  echo "  ssh pi@raspberry.local 'sudo systemctl restart bossa && journalctl -u bossa -n 20'"
fi

echo "All pre-push gates passed."
