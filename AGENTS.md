# AGENTS.md

This file provides guidance to AI coding agents (Antigravity, Gemini, Claude, Cursor, and other agents) when working with code in this repository.

@.guidelines/workflow/sdd.md
@.guidelines/workflow/integration.md
@.guidelines/workflow/tdd.md
@.guidelines/agents/writing.md
@.guidelines/style/naming.md
@.guidelines/languages/cpp.md
@.guidelines/languages/cmake.md
@.guidelines/languages/sh.md

For BOSSA's own project context, ecosystem, embedded/hardware policy, and
merge policy, read [CONTRIBUTING.md](CONTRIBUTING.md). For BOSSA-specific
coding notes, read [docs/guidelines.md](docs/guidelines.md).

Conflict order: direct maintainer request > CONTRIBUTING.md > `.guidelines/`
> `docs/guidelines.md` > `docs/specification.md` > modern C++ best
practices (C++ Core Guidelines).

## Quick reference: Common tasks

| Task | Command |
|---|---|
| Format C++ code | `./scripts/clang.sh` |
| Check formatting gate | `bash scripts/check/formatting.sh` |
| Build native (x86_64) | `./scripts/build.sh` |
| Run unit tests (ctest) | `cd build && ctest --output-on-failure -V` |
| Fast unit test runner | `./scripts/test/unit.sh` |
| Cross-compile (ARM64 / Pi 5) | `./scripts/build.sh -t toolchain-arm64.cmake` |
| Full pre-push quality gate | `bash scripts/check/pre_push.sh` |
| Fast pre-push gate (no cross) | `bash scripts/check/pre_push.sh --skip-cross` |
| Deploy to Pi 5 | `./scripts/sync.sh -t pi@raspberry.local` |
| Pi 5 hardware smoke test | `./scripts/test/pi5_bme280_smoke.sh --target pi@raspberry.local` |

## Architecture at a glance

BOSSA is a modular C++20 embedded framework for IoT edge devices that publishes telemetry to SQLite: a local offline buffer on the device, and a remote Cloudflare D1 database via a Cloudflare Worker.

1. **Edge Runtime (`bossa-daemon`)**:
   - `bossa_core`: Service daemon lifecycle (`bossa::core::Service`), YAML configuration loader (`bossa::core::Config`).
   - `bossa_io`: Hardware abstractions (`GpioController`, `I2cBus`, `LibgpiodGpio`, `LinuxI2cBus`).
   - `bossa_drivers`: Sensor driver interface (`Driver`, `ReadResult`), static registry (`Registry`), drivers (`bme280`, `sim`).
   - `bossa_telemetry`: Non-allocating sample representation (`Sample`, `StoredSample`), deadline `Scheduler`, priority-aware `RingBuffer`.
   - `bossa_storage`: Local SQLite queue (`LocalStore`) with WAL mode for offline buffering.
   - `bossa_sync`: Declarative sync policy (`UploadPolicy`), HTTP client abstraction (`CurlHttpClient`), `HttpUploader`.
   - `bossa_runtime`: Complete pipeline orchestrator (`TelemetryRuntime`) connecting drivers → scheduler → ring buffer → local store → HTTP uploader.

2. **Cloud Ingress (Phase 4)**:
   - BOSSA Cloudflare Worker (`POST /api/v1/telemetry`) ingesting JSON batches into Cloudflare D1 (SQLite).

## Pre-push gates (mandatory)

Before every `git push` on a PR branch, agents **must** run at minimum:

```bash
bash scripts/check/formatting.sh
./scripts/build.sh
```

Do not push or update a PR until both pass. When time allows, prefer the
full local gate before push:

```bash
bash scripts/check/pre_push.sh
```

## Agent environment instructions

- **Dual-target builds:** BOSSA is developed on x86_64 and deployed on
  ARM64 (Raspberry Pi 5). Always verify the native build; run
  cross-compile via `bash scripts/check/pre_push.sh` or
  `./scripts/build.sh -t toolchain-arm64.cmake` before claiming done.
- **No Pi attached:** cloud and local dev host environments typically do not
  have a Raspberry Pi 5 connected. Run GTest with mocked `bossa::io` interfaces.
  Document the Pi 5 smoke procedure in the PR test plan for human execution;
  never claim hardware validation without evidence.
- **Dependencies:** run `./scripts/setup.sh` if the compiler or toolchain
  is missing.
- **Ecosystem:** BOSSA syncs telemetry to a BOSSA-owned Cloudflare Worker
  + D1 (SQLite). Edge keeps a local SQLite offline buffer; uploads go
  over HTTPS.
- **Embedded constraints:** No heap allocation in hot paths (`read()`,
  `write()`, scheduler loop, ring buffer). No exceptions in hot paths or signal
  handlers.
