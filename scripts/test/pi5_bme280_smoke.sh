#!/bin/bash
# Phase 2 Pi 5 BME280 hardware smoke test launcher (manual / on-device; not CI).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
source "${PROJECT_DIR}/scripts/env.sh"

usage() {
  echo "Usage: $(basename "$0") [options]"
  echo "  --target HOST          SSH target (e.g. pi@raspberry.local). Omit to run locally."
  echo "  --bus PATH             I2C device (default: /dev/i2c-1)"
  echo "  --address ADDR         7-bit address (default: 0x76)"
  echo "  --duration SECONDS     How long to poll (default: 10)"
  echo "  --install-dir PATH     BOSSA install prefix on Pi (default: /opt/bossa)"
  echo ""
  echo "See docs/hardware/pi5-bme280-smoke-test.md for wiring and prerequisites."
  exit 0
}

TARGET=""
I2C_BUS="/dev/i2c-1"
I2C_ADDRESS="0x76"
DURATION_SECONDS=10
INSTALL_DIR="/opt/bossa"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --target) TARGET="${2:-}"; shift 2 ;;
    --bus) I2C_BUS="${2:-}"; shift 2 ;;
    --address) I2C_ADDRESS="${2:-}"; shift 2 ;;
    --duration) DURATION_SECONDS="${2:-}"; shift 2 ;;
    --install-dir) INSTALL_DIR="${2:-}"; shift 2 ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

SMOKE_BIN="${INSTALL_DIR}/bin/bossa-bme280-smoke"

run_smoke_locally() {
  echo "=== Checking I2C bus ${I2C_BUS} ==="
  if ! command -v i2cdetect &>/dev/null; then
    echo "Warning: i2c-tools not installed (sudo apt-get install i2c-tools)"
  elif [[ -e "${I2C_BUS}" ]]; then
    i2cdetect -y "${I2C_BUS##*/i2c-}" || true
  else
    echo "Warning: ${I2C_BUS} not found"
  fi

  if [[ ! -x "${SMOKE_BIN}" ]] && [[ -x "${PROJECT_DIR}/build/final/bin/bossa-bme280-smoke" ]]; then
    SMOKE_BIN="${PROJECT_DIR}/build/final/bin/bossa-bme280-smoke"
  fi

  if [[ ! -x "${SMOKE_BIN}" ]]; then
    echo "Error: ${SMOKE_BIN} not found. Deploy first: ./scripts/sync.sh -t <pi>"
    exit 1
  fi

  echo "=== Polling BME280 for ${DURATION_SECONDS}s (syslog + stderr) ==="
  timeout "${DURATION_SECONDS}" \
    sudo "${SMOKE_BIN}" --foreground \
      --bus "${I2C_BUS}" \
      --address "${I2C_ADDRESS}" \
      --interval-seconds 1 || true

  echo "=== Recent syslog (bossa-bme280-smoke) ==="
  sudo journalctl -t bossa-bme280-smoke -n 20 --no-pager || true
}

run_smoke_remote() {
  echo "=== Running smoke test on ${TARGET} ==="
  ssh "${TARGET}" \
    "INSTALL_DIR='${INSTALL_DIR}' I2C_BUS='${I2C_BUS}' I2C_ADDRESS='${I2C_ADDRESS}' DURATION_SECONDS='${DURATION_SECONDS}' bash -s" <<'REMOTE'
set -euo pipefail
SMOKE_BIN="${INSTALL_DIR}/bin/bossa-bme280-smoke"

if command -v i2cdetect &>/dev/null && [[ -e "${I2C_BUS}" ]]; then
  BUS_NUM="${I2C_BUS##*/i2c-}"
  echo "=== i2cdetect on bus ${BUS_NUM} ==="
  sudo i2cdetect -y "${BUS_NUM}" || true
fi

if [[ ! -x "${SMOKE_BIN}" ]]; then
  echo "Error: ${SMOKE_BIN} missing on Pi. Run ./scripts/sync.sh from dev host first."
  exit 1
fi

echo "=== Polling BME280 for ${DURATION_SECONDS}s ==="
timeout "${DURATION_SECONDS}" \
  sudo "${SMOKE_BIN}" --foreground \
    --bus "${I2C_BUS}" \
    --address "${I2C_ADDRESS}" \
    --interval-seconds 1 || true

echo "=== Recent syslog ==="
sudo journalctl -t bossa-bme280-smoke -n 20 --no-pager || true
REMOTE
}

if [[ -n "${TARGET}" ]]; then
  run_smoke_remote
else
  run_smoke_locally
fi

echo "Smoke test finished. See docs/hardware/pi5-bme280-smoke-test.md for troubleshooting."
