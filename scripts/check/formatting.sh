#!/bin/bash
# BOSSA formatting gate — must pass before every push.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

cd "${PROJECT_DIR}"
./scripts/clang.sh
git diff --exit-code

echo "Formatting check passed."
