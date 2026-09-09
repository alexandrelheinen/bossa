# BOSSA-specific coding notes

The shared baseline for C++ style, naming, comments, and error handling
lives in [.guidelines/languages/cpp.md](../.guidelines/languages/cpp.md),
[.guidelines/style/naming.md](../.guidelines/style/naming.md), and
[.guidelines/style/errors.md](../.guidelines/style/errors.md). This file
covers only what is specific to BOSSA: directory layout, the embedded/
real-time constraints, and build/deployment conventions. Workflow, SDD,
V-cycle, and merge policy live in [CONTRIBUTING.md](../CONTRIBUTING.md).

## 1. File and Package Structure

### Directory Layout

```
bossa/
├── include/bossa/         ← Public headers
│   ├── core/              ← Service, config loader, lifecycle
│   ├── io/                ← GPIO, I2C, SPI abstractions
│   ├── drivers/           ← Driver interface and registry
│   ├── telemetry/         ← Sample types, ring buffer, scheduler
│   ├── storage/           ← Edge SQLite local store
│   └── sync/              ← Upload policy and HTTP uploader
├── src/                   ← Implementation files (.cpp)
├── drivers/               ← Built-in and example driver adapters
├── workers/               ← (Phase 4) Cloudflare Worker + D1
├── tests/                 ← Unit tests (GTest)
├── config/                ← systemd units, example YAML, D1 migrations
├── scripts/               ← Build, deployment, and utility scripts
├── docs/                  ← Specification, roadmap, guidelines
└── CMakeLists.txt         ← Build configuration
```

See [specification.md](specification.md) for module responsibilities and
[roadmap.md](roadmap.md) for the phased delivery plan.

### Namespace convention

Namespace mirrors directory structure; all BOSSA code lives under the
`bossa::` namespace. Macros use a `BOSSA_` prefix (e.g. `BOSSA_VERSION`).

## 2. Testing

GTest, tests mirror `include/bossa/` under `tests/`. Use `GTEST_SKIP()` or
disabled tests with a documented reason for methods not yet implemented;
enable once the stub is filled.

- **Unit tests**: individual classes/functions in isolation, hardware
  mocked.
- **Integration tests**: interactions between components (may require
  real hardware or simulators).
- **Hardware tests**: deploy to Raspberry Pi and run smoke tests.
- **Native build** (x86_64): unit tests locally during development.
- **Cross-compiled** (ARM64): deploy to target hardware for integration
  testing. QEMU for ARM64 emulation when hardware is unavailable (limited
  usefulness for I/O testing).

## 3. Embedded and Real-Time Considerations

- **CPU**: avoid busy-waiting; use sleep, condition variables, or
  interrupts.
- **I/O bandwidth**: cache sensor readings when appropriate; do not poll
  unnecessarily.
- **Minimize allocations** in hot paths (pre-allocate buffers).
- **Avoid blocking** in time-critical loops.
- **Signal handling**: use `sigaction()` over `signal()` for portability.
- **Daemon conventions**: follow the daemonization pattern in the
  `Service` class.
- **Logging**: `syslog()` for daemon logs; avoid `std::cout` in production
  daemons.
- Abstract hardware interfaces behind clean APIs (dependency injection /
  factory patterns for testability), e.g.:
  ```cpp
  class IGPIOController {
  public:
      virtual ~IGPIOController() = default;
      virtual bool read_pin(int pin) = 0;
      virtual void write_pin(int pin, bool value) = 0;
  };
  ```

See [CONTRIBUTING.md § Defensive programming](../CONTRIBUTING.md#defensive-programming)
for the full error-handling-by-layer policy this section builds on.

## 4. Build and Dependency Management

- Modern CMake (3.16+); `set(CMAKE_CXX_STANDARD 20)`.
- Minimize external dependencies for embedded targets; prefer header-only
  or system libraries available via apt; document non-standard
  dependencies in README; use `find_package()` for third-party libraries.
- Cross-compilation via CMake toolchain files (e.g. `toolchain-arm64.cmake`);
  test toolchain files regularly to avoid drift. Cross-compiled binaries
  go to `build/final/bin/bossa`.

## 5. systemd Integration

- Service files live in `config/` (e.g. `bossa.service`). Use
  `Type=forking` if the service daemonizes itself, `Type=simple` or
  `Type=notify` otherwise. Include `Restart=on-failure`.
- Log levels: `LOG_DEBUG` (verbose, disabled in production), `LOG_INFO`
  (normal operation), `LOG_WARNING` (recoverable), `LOG_ERR` (needs
  attention), `LOG_CRIT` (service may terminate).
- Deploy via `scripts/sync.sh`: binary, service file, and config files.

## 6. Pre-flight Checklist

Before finishing any implementation task, all of the following must pass
locally:

```bash
# 1. Native build (x86_64)
./scripts/build.sh

# 2. Run unit tests (if available)
cd build && ctest -V

# 3. Code formatting — zero issues
bash scripts/check/formatting.sh

# 4. Cross-compile for ARM64
./scripts/build.sh -t toolchain-arm64.cmake

# 5. (Optional) Deploy and smoke test on Raspberry Pi 5
./scripts/sync.sh -t pi@raspberry.local
ssh pi@raspberry.local 'sudo systemctl restart bossa && sleep 2 && sudo systemctl status bossa'
```

Or run all gates at once: `bash scripts/check/pre_push.sh`.
