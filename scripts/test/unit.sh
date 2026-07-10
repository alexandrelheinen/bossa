#!/bin/bash
# Run BOSSA GTest unit tests (native x86_64). Not used in CI — CI calls ctest directly.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

usage() {
  echo "Usage: $(basename "$0") [--rebuild] [--verbose]"
  echo "  --rebuild   Run ./scripts/build.sh before ctest"
  echo "  --verbose   Pass -V to ctest"
  exit 0
}

REBUILD=0
CTEST_ARGS=(--output-on-failure)

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rebuild) REBUILD=1 ;;
    --verbose) CTEST_ARGS+=(-V) ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
  shift
done

cd "${PROJECT_DIR}"

if [[ "${REBUILD}" -eq 1 ]] || [[ ! -f build/CTestTestfile.cmake ]]; then
  echo "=== Building (native) ==="
  CXX="${CXX:-g++}" ./scripts/build.sh
fi

if [[ ! -x build/final/bin/bossa_tests ]]; then
  echo "Error: build/final/bin/bossa_tests not found. Run with --rebuild."
  exit 1
fi

echo "=== Running unit tests ==="
(cd build && ctest "${CTEST_ARGS[@]}")
echo "Unit tests passed."
