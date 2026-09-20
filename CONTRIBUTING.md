# Contributing to BOSSA

This document is BOSSA's own project context: ecosystem, quality gates,
embedded/hardware policy, and merge policy. Generic method (Spec-Driven
Development, the V-cycle, TDD), writing, and naming guidelines live in
[.guidelines/](.guidelines/), a shared submodule, not here.

Do not duplicate rules from the submodule in other files. `AGENTS.md`,
`CLAUDE.md`, and `.github/copilot-instructions.md` exist only as entry
points that point to `.guidelines/` and this file.

For **coding conventions** (C++20 style, naming, Doxygen, embedded
constraints), see [docs/guidelines.md](docs/guidelines.md) for what is
specific to BOSSA, and
[.guidelines/languages/cpp.md](.guidelines/languages/cpp.md) for the shared
baseline. This file covers *how work is planned, verified, and merged*.

## Table of contents

1. [Ecosystem context](#ecosystem-context)
2. [Development setup](#development-setup)
3. [Method](#method)
4. [Requirement traceability](#requirement-traceability)
5. [Quality gates](#quality-gates)
6. [Definition of Ready and Definition of Done](#definition-of-ready-and-definition-of-done)
7. [Pull request workflow](#pull-request-workflow)
8. [Defensive programming](#defensive-programming)
9. [Embedded contributions](#embedded-contributions)
10. [Raspberry Pi 5 hardware validation](#raspberry-pi-5-hardware-validation)
11. [Rules for AI agents](#rules-for-ai-agents)
12. [Reference documents](#reference-documents)
13. [Pre-merge checklist](#pre-merge-checklist)

---

## Ecosystem context

BOSSA is one project in a family of repositories that share the same SDD +
V-cycle methodology, refined for AI-assisted development. Understanding that
context helps contributors apply the method consistently.

| Project | Role | SDD maturity |
| --- | --- | --- |
| **[Personal website](https://alexandrelheinen.pages.dev)** | Publishes methodology articles and project portfolio entries | Explains *why* SDD works with agents ([SDD and agentic AI for production-quality code](https://alexandrelheinen.pages.dev/articles/2026-04-22-ai-agents-sdd/)) |
| **[Luthier](https://github.com/alexandrelheinen/luthier)** | Python photogrammetry library | Reference implementation: `CONTRIBUTING.md` as constitution, `AC-*` traceability, governance CI |
| **[Freshy](https://freshy-25e.pages.dev/explore)** | Mobile-first cooling-map app (Next.js, Cloudflare) | Product-level SDD in a private repo; SQL data model for telemetry and analytics |
| **[ARCO](https://github.com/alexandrelheinen/arco)** | Motion planning and control algorithms | Algorithm library; consumed by FRET for planning |
| **[FRET](https://github.com/alexandrelheinen/fret)** | ROS 2 end-to-end robotic planning + control stack | Reference constitution for SDD + 4-level V-cycle + agent policy |
| **BOSSA (this repo)** | Edge runtime + Worker/D1 telemetry for IoT on ARM Linux | C++20 embedded framework; Pi 5 first target; SQLite (edge + D1) |

### What BOSSA provides

BOSSA is a **modular C++20 embedded framework** with two deployable binaries:

| Artifact | Runs on | Role |
| --- | --- | --- |
| `bossa-daemon` | Raspberry Pi 5 (edge) | Driver hosting, sampling, local SQLite buffer, HTTPS upload |
| BOSSA Worker + D1 | Cloudflare | REST ingress, authentication, D1 (SQLite) writes |

The specification stack:

| Level | Specification artifacts | Validation artifacts |
| --- | --- | --- |
| 1 — Functional | [docs/specification.md](docs/specification.md) §1–2, [docs/roadmap.md](docs/roadmap.md) phase acceptance criteria | Hardware smoke tests, integration tests |
| 2 — Architecture | [README.md § Architecture](../README.md#architecture-overview), [docs/specification.md](docs/specification.md) §3–11 | Cross-compile CI, API contract tests |
| 3 — Module API | Public headers under `include/bossa/` | `tests/` mirroring `include/bossa/` (GTest) |
| 4 — Implementation | `src/`, `drivers/`, `workers/` | Full CI + Pi 5 deployment smoke |

Project status and delivery sequence: [docs/roadmap.md](docs/roadmap.md).
Architecture and technology stack: [README.md](../README.md).

---

## Development setup

**Requirements:**

- Ubuntu 22.04+ or Debian-based Linux (development host, x86_64)
- GCC 10+ or Clang 14+ with C++20 support
- CMake 3.16+
- Git
- For cross-compilation: `gcc-aarch64-linux-gnu`, `g++-aarch64-linux-gnu`
- For hardware validation: SSH access to a Raspberry Pi 5 running Raspberry Pi OS

```bash
git clone https://github.com/alexandrelheinen/bossa.git
cd bossa
./scripts/setup.sh
./scripts/build.sh
```

### Dual-target workflow

BOSSA is developed on **x86_64** and deployed on **ARM64 (Pi 5)**:

| Target | Command | Purpose |
| --- | --- | --- |
| Native (x86_64) | `./scripts/build.sh` | Fast iteration, GTest unit tests |
| Cross (ARM64) | `./scripts/build.sh -t toolchain-arm64.cmake` | Production binary for Pi 5 |
| Deploy | `./scripts/sync.sh -t pi@raspberry.local` | Copy tarball to device |

Run the same checks CI runs before opening or updating a pull request:

```bash
bash scripts/check/pre_push.sh              # full gate
bash scripts/check/pre_push.sh --skip-cross # formatting + native build + tests only
```

---

## Method

Spec-Driven Development, the V-cycle, and TDD are defined once in
[.guidelines/workflow/sdd.md](.guidelines/workflow/sdd.md),
[.guidelines/workflow/integration.md](.guidelines/workflow/integration.md),
and [.guidelines/workflow/tdd.md](.guidelines/workflow/tdd.md). Follow
those. BOSSA's own additions:

- The project-level specification lives in `docs/specification.md` and
  `docs/roadmap.md`; a feature-level spec is a GitHub issue or PR
  description with acceptance criteria.
- Default rigor level: spec-first minimum, spec-anchored when module
  boundaries, driver interfaces, or sync policies change.
- The V-cycle's four levels map to concrete BOSSA locations:

  | Level | Artifact | BOSSA location | Verification |
  | --- | --- | --- | --- |
  | 1 — Functional | Goals, sync policies, driver behavior, server API contract | `docs/specification.md`, `docs/roadmap.md` | Pi 5 smoke test, Worker + D1 end-to-end |
  | 2 — Architecture | Module decomposition, driver interface, sync engine, REST API | `README.md § Architecture`, `docs/specification.md` §5-10 | Cross-compilation succeeds, API contract tests |
  | 3 — Module API | Public APIs: class declarations, Doxygen, stub bodies | `include/bossa/<module>/` | GTest unit tests written with the stubs, in `tests/` mirroring `include/bossa/` |
  | 4 — Implementation | Algorithms, driver adapters, Worker/D1 wiring | `src/`, `drivers/`, `workers/` | Full test suite, coverage >= 90%, Pi 5 smoke test |

  Rule: no work at a level starts until that level's artifact exists (no
  Level 2 work on new behavior until the spec section and roadmap
  acceptance criteria exist; no Level 3 work on a new module boundary
  until `docs/specification.md` defines the public interface).
- Agent instruction files (`AGENTS.md`, `CLAUDE.md`,
  `.github/copilot-instructions.md`) must only point to `.guidelines/` and
  this document, per [.guidelines/agents/context.md](.guidelines/agents/context.md).

---

## Requirement traceability

Requirements must be traceable in both directions: from an acceptance criterion
down to the test that proves it, and from any test back to the criterion.

| Identifier | Document | Example |
| --- | --- | --- |
| `FR-<DOMAIN>-<NN>` | [docs/specification.md](docs/specification.md) | `FR-DRV-01` (driver reads sensor) |
| `FR-SYNC-<NN>` | [docs/specification.md](docs/specification.md) §9 | `FR-SYNC-03` (offline SQLite queue) |
| `FR-SRV-<NN>` | [docs/specification.md](docs/specification.md) §10 | `FR-SRV-01` (REST telemetry ingress) |
| Phase | [docs/roadmap.md](docs/roadmap.md) | Phase 2 acceptance criteria |

**Domain codes:** `SYS` (system), `DRV` (drivers), `IO` (GPIO/I2C/SPI),
`TEL` (telemetry), `SYNC` (upload), `SRV` (server), `CFG` (configuration).

**Rules:**

- Reference `FR-*` and roadmap phase in issue text, PR descriptions, or GTest
  comments when asserting observable behavior.
- Every new functional requirement needs a test or hardware smoke procedure; a
  requirement with no verification is an incomplete spec.
- Do not silently disable tests (`GTEST_SKIP`, `DISABLED_`) without a comment
  naming the blocking requirement or tracking issue.

When adding requirements to [docs/specification.md](docs/specification.md), use
the `FR-<DOMAIN>-<NN>` format and update the traceability table in the PR.

---

## Quality gates

### Local validation

```bash
# Recommended: all gates before push
bash scripts/check/pre_push.sh

# Faster iteration (no cross-compile)
bash scripts/check/pre_push.sh --skip-cross

# Or step by step:
bash scripts/check/formatting.sh
./scripts/build.sh
cd build && ctest --output-on-failure -V
./scripts/build.sh -t toolchain-arm64.cmake
```

### CI workflows

| Workflow | Trigger | What it checks |
| --- | --- | --- |
| `formatting.yml` | PR | clang-format on `src/`, `include/`, `tests/` |
| `build-and-test.yml` | PR | Native x86_64 build, `ctest`, ARM64 cross-compile, artifact |

### Coverage and style

- GTest coverage target: **>= 90%** on `bossa_core`, `bossa_telemetry`,
  `bossa_sync`, `bossa_server` (when those libraries exist).
- Embedded: no exceptions or heap allocation in hot paths (scheduler,
  `read()`, `write()`, signal handlers).
- Formatting, Doxygen, and naming follow
  [.guidelines/languages/cpp.md](.guidelines/languages/cpp.md) and
  [.guidelines/style/naming.md](.guidelines/style/naming.md).

---

## Definition of Ready and Definition of Done

### Definition of Ready (before coding)

- [ ] Intent, scope, and **acceptance criteria** are written (GitHub issue or PR).
- [ ] Rigor level chosen (spec-first / spec-anchored / spec-as-source).
- [ ] Affected V-cycle levels and test levels identified.
- [ ] Linked `FR-*` ids and roadmap phase when behavior changes.
- [ ] No undocumented new runtime dependency or public API break.
- [ ] Hardware requirements identified (which Pi peripherals, which I2C address).

### Definition of Done (before merge)

- [ ] All acceptance criteria verified by GTest, integration test, or documented
  Pi 5 smoke procedure.
- [ ] Focused commits; conventional commit messages.
- [ ] All required CI jobs green on the PR branch.
- [ ] `docs/` updated when requirements, architecture, or interfaces changed.
- [ ] Native build and ARM64 cross-compile pass.
- [ ] No quality gate weakened to pass.
- [ ] Human reviewer merged (agents do not self-merge).
- [ ] Hardware-validated when the change touches I/O, drivers, or timing.

---

## Pull request workflow

1. Branch from `main` with a descriptive name (e.g. `feat/bme280-driver`,
   `fix/scheduler-deadline`).
2. Make **focused commits**—one logical step each.
3. Open a PR (see [.guidelines/templates/pr.md](.guidelines/templates/pr.md)
   for the base template) with two BOSSA-specific fields added:
   - **Traceability** — `FR-*` / roadmap phase references when applicable.
   - **Hardware plan** — Pi 5 smoke steps when I/O is involved.
4. Ensure CI is green before requesting review.
5. Address feedback in new commits (avoid force-push to `main`).

Commit format follows
[.guidelines/workflow/commits.md](.guidelines/workflow/commits.md).

---

## Defensive programming

BOSSA runs on embedded Linux where failures are costly and hard to debug.
**Defensive programming** means assuming inputs, hardware, and the environment
can fail — and handling those failures explicitly at the point they occur.

This section is the project-wide policy. Module-specific notes (for example
in [docs/guidelines.md](docs/guidelines.md)) apply the same rules to concrete
modules.

### Principles

| Principle | Rule | Example |
| --- | --- | --- |
| **Validate early** | Reject invalid config and CLI input before the main loop | `Config::load()` returns an error result; daemon exits with code 1 |
| **Check every syscall** | Never ignore return values from POSIX calls | `if (sigaction(...) < 0) { syslog(LOG_ERR, ...); return EXIT_FAILURE; }` |
| **Fail fast, log clearly** | On unrecoverable error: `LOG_ERR` with context, then exit or return error | Invalid YAML → `LOG_ERR` with file path and reason |
| **No silent defaults** | Do not substitute guessed values for missing required config | Missing `node.id` is an error, not an empty string |
| **Bound all inputs** | Reject oversize files, empty paths, and out-of-range numbers at parse time | Config file size limit; `sample_rate_hz >= 0` |
| **Signal safety** | Handlers set only `volatile sig_atomic_t` flags | No `malloc`, `mutex`, or `syslog` inside handlers |
| **No hot-path surprises** | No exceptions or heap allocation in `read()`, `write()`, scheduler, or signal paths | Pre-allocate buffers at `configure()` time |
| **Test the failure paths** | Every validation rule and error branch needs a GTest | `ConfigTest.InvalidVersionReturnsError` |

### Error handling by layer

| Layer | Allowed mechanism | Not allowed |
| --- | --- | --- |
| Startup (config, CLI, daemonize) | Return codes, result types, `LOG_ERR`, `exit(1)` | Ignored errors, empty catch blocks |
| Service main loop | Return codes, bounded retries with backoff | Exceptions, unbounded recursion |
| Driver hot path (`read()` / `write()`) | Return codes, `SampleQuality::kBad` | Exceptions, `new` / `delete` |
| Signal handlers | `sig_atomic_t` flag assignment only | Any other libc or C++ call |

### Result types over exceptions (startup and config)

Prefer explicit result types for operations that can fail during startup:

```cpp
// bossa::core::ConfigLoadResult — success carries Config, failure carries message
auto result = bossa::core::load_config(path);
if (!result.ok()) {
  syslog(LOG_ERR, "config: %s", result.error().c_str());
  return EXIT_FAILURE;
}
```

Exceptions are acceptable in test code and in one-time initialization that is
not on a real-time path, but production daemon startup should be provably
fail-fast without relying on catch-all handlers.

### Logging on error

- Use `syslog(LOG_ERR, ...)` for errors that stop or degrade the service.
- Include **what** failed and **why** (file path, syscall name, validation rule).
- Do not log secrets (API keys, connection strings with passwords).
- Use `LOG_WARNING` for recoverable conditions (retry, skipped sample).
- Use `LOG_DEBUG` for verbose diagnostics; disabled in production builds when
  appropriate.

### RAII and resource cleanup

- Open file descriptors and syslog in RAII wrappers or dedicated setup/teardown
  functions with a single exit path.
- Close FDs on all error branches — prefer early `return` after cleanup over
  deep nesting.
- Destructors must not throw.

### Unit-test requirements for defensive code

When adding validation or error handling:

1. Add a GTest that triggers the failure (missing field, bad version, syscall
   failure simulated via injection or test doubles).
2. Assert the observable outcome: exit code, error message, or returned error
   type — not only that the code "does not crash".
3. Reference the roadmap phase or `FR-*` id in the test comment when asserting
   specified behavior.

### Agent checklist

Before marking defensive work complete:

- [ ] Every new syscall or library call checks its return value.
- [ ] Every new config field has a validation rule and a failing test.
- [ ] Signal handlers remain async-signal-safe.
- [ ] No secrets in log messages.
- [ ] Error paths are covered by GTest (not only the happy path).

---

## Embedded contributions

### Adding a hardware driver

1. Create adapter in `drivers/<name>/` implementing `bossa::drivers::Driver`
2. Public interface documented in [docs/specification.md](docs/specification.md) §7
3. Unit tests in `tests/drivers/<name>_test.cpp` with mock `bossa::io::I2cBus`
4. Register via `BOSSA_REGISTER_DRIVER` or dynamic `.so` plugin
5. Example YAML channel entry in `config/examples/`
6. Pi 5 smoke test documented in PR test plan

### Adding an I/O abstraction

1. Virtual interface in `include/bossa/io/<bus>.hpp`
2. Linux implementation in `src/io/<bus>_linux.cpp`
3. Mock in `tests/mocks/mock_<bus>.hpp`
4. GTest in `tests/io/<bus>_test.cpp`
5. No direct hardware access outside `bossa::io` implementations

### Adding a Worker / D1 endpoint

1. Route handler in `workers/` (Phase 4 Cloudflare Worker)
2. Update [docs/specification.md](docs/specification.md) §10 REST API table
3. Contract test with `wrangler dev` or Miniflare
4. D1 migration + integration test when CI supports it

### Adding a dependency

1. Add to `CMakeLists.txt` with `find_package()` or `FetchContent`
2. Add apt package to `scripts/setup.sh`
3. Document in [docs/specification.md](docs/specification.md) §4
4. Verify availability on Raspberry Pi OS Bookworm
5. Verify cross-compilation toolchain can link the library

### systemd service changes

1. Update unit file in `config/bossa.service`
2. Document in PR if `Type=`, `User=`, or `ExecStart=` changes
3. Test on Pi: `sudo systemctl daemon-reload && sudo systemctl restart bossa`

---

## Raspberry Pi 5 hardware validation

BOSSA targets **Raspberry Pi 5** running **Raspberry Pi OS Bookworm (64-bit)**.
Hardware validation is required when a change touches drivers, I/O, timing, or
deployment.

### Standard smoke procedure

See **[docs/hardware/pi5-bme280-smoke-test.md](docs/hardware/pi5-bme280-smoke-test.md)**
for BME280 wiring, datasheet links, and the Phase 2 poll harness.

```bash
# On development host — unit tests (local, not CI)
./scripts/test/unit.sh

# Cross-compile and deploy
./scripts/build.sh -t toolchain-arm64.cmake
./scripts/sync.sh -t pi@raspberry.local

# Pi 5 BME280 smoke (from dev host over SSH)
./scripts/test/pi5_bme280_smoke.sh --target pi@raspberry.local

# On Pi — foreground BME280 poll (~1 Hz syslog)
sudo /opt/bossa/bin/bossa-bme280-smoke --foreground --bus /dev/i2c-1 --address 0x76
sudo journalctl -t bossa-bme280-smoke -f

# Daemon sanity check (config + heartbeat loop)
sudo /opt/bossa/bin/bossa-daemon --foreground
```

### What to verify

| Change type | Evidence |
| --- | --- |
| Driver | Correct sample values in syslog; see [Pi 5 BME280 smoke test](docs/hardware/pi5-bme280-smoke-test.md) |
| Scheduler | Samples arrive at configured `sample_rate_hz` (±10%) |
| Sync / upload | Row appears in D1 `telemetry_points` table |
| Offline mode | Disconnect network; samples in SQLite; reconnect; rows uploaded |
| GPIO / actuator | Pin state matches command; no glitches on startup |

### Cloud agent limitations

Cloud development environments typically **do not have a Pi 5 attached**. Agents
must:

- Run native build, formatting, and GTest locally.
- Run ARM64 cross-compile locally.
- Document the Pi 5 smoke procedure in the PR test plan for human execution.
- Never claim hardware validation is complete without evidence.

---

## Rules for AI agents

These rules apply to Cursor agents, Copilot, Claude Code, and any automated
contributor. General agent behavior (evidence, no fabrication, git safety,
communication) follows
[.guidelines/agents/claude.md](.guidelines/agents/claude.md). BOSSA adds:

### Execution order

1. Confirm the spec / acceptance criteria exist for the V-cycle level being
   touched; update `docs/` when Level 1-2 changes.
2. Headers + GTest together when adding APIs (Level 3). Mock all hardware.
3. Implement (Level 4); run `scripts/check/pre_push.sh` (or `--skip-cross`
   when iterating).
4. Push only after `scripts/check/formatting.sh` passes; ensure CI green;
   document Pi smoke for I/O changes.

### Embedded-specific rules

- **No heap allocation** in `read()`, `write()`, scheduler loop, or ring
  buffer.
- **No exceptions** in hot paths or signal handlers.
- **Mock hardware** in unit tests, use `bossa::io` virtual interfaces.
- **syslog** for daemon logging, not `std::cout`.
- **Cross-compile** every PR that changes production code.

---

## Reference documents

When an external guide conflicts with this file, `.guidelines/`, or
[docs/guidelines.md](docs/guidelines.md), **this repository wins**.

| Document | Role |
| --- | --- |
| [.guidelines/](.guidelines/) | Shared method, writing, naming, and per-language style (submodule) |
| [docs/specification.md](docs/specification.md) | Technical requirements, APIs, libraries, schemas |
| [docs/roadmap.md](docs/roadmap.md) | Phased delivery plan and acceptance criteria |
| [README.md § Architecture](../README.md#architecture-overview) | Level 2 — system design |
| [docs/guidelines.md](docs/guidelines.md) | BOSSA-specific coding notes |

---

## Pre-merge checklist

Every contributor (human or agent) must confirm before merge, beyond the
shared [.guidelines/templates/pr.md](.guidelines/templates/pr.md) checklist:

- [ ] `FR-*` / roadmap traceability updated when behavior changed.
- [ ] `./scripts/build.sh` passes (native x86_64) and
      `./scripts/build.sh -t toolchain-arm64.cmake` passes.
- [ ] `ctest` passes when `tests/` is populated.
- [ ] Pi 5 smoke documented or executed when I/O changed.
- [ ] No exceptions or heap allocation introduced in hot paths.

If any item fails, fix forward, do not merge.

---

## License

By contributing, you agree that your contributions will be licensed under the
MIT License.
