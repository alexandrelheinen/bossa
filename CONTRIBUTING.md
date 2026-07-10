# Contributing to BOSSA

This document is the **single source of truth** for how humans and AI agents
contribute to BOSSA (Base Operating System for Sensors and Actuators). It
defines the development constitution: Specification-Driven Development (SDD), the
4-level V-cycle software development lifecycle, embedded quality gates, and
agent policy.

Do not duplicate these rules in other files. `AGENTS.md`, `CLAUDE.md`, and
`.github/copilot-instructions.md` exist only as entry points that point here.

For **coding conventions** (C++20 style, naming, Doxygen, embedded constraints),
see [docs/guidelines.md](docs/guidelines.md). That file covers *how* code is
written; this file covers *how work is planned, specified, verified, and merged*.

## Table of contents

1. [Ecosystem context](#ecosystem-context)
2. [Development setup](#development-setup)
3. [Spec-driven development (SDD)](#spec-driven-development-sdd)
4. [BOSSA development stages (4-level V-cycle)](#bossa-development-stages-4-level-v-cycle)
5. [Combining SDD, V-cycle, and TDD](#combining-sdd-v-cycle-and-tdd)
6. [Constraint layers for AI-assisted work](#constraint-layers-for-ai-assisted-work)
7. [Requirement traceability](#requirement-traceability)
8. [Quality gates](#quality-gates)
9. [Definition of Ready and Definition of Done](#definition-of-ready-and-definition-of-done)
10. [Pull request workflow](#pull-request-workflow)
11. [Embedded contributions](#embedded-contributions)
12. [Raspberry Pi 5 hardware validation](#raspberry-pi-5-hardware-validation)
13. [Rules for AI agents](#rules-for-ai-agents)
14. [Reference documents](#reference-documents)
15. [Pre-merge checklist](#pre-merge-checklist)

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
| **BOSSA (this repo)** | Edge runtime + telemetry server for IoT on ARM Linux | C++20 embedded framework; Pi 5 first target; PostgreSQL sync |

### What BOSSA provides

BOSSA is a **modular C++20 embedded framework** with two deployable binaries:

| Binary | Runs on | Role |
| --- | --- | --- |
| `bossa-daemon` | Raspberry Pi 5 (edge) | Driver hosting, sampling, local SQLite buffer, HTTPS upload |
| `bossa-server` | Server / cloud VM | REST ingress, authentication, PostgreSQL writer |

The specification stack:

| Level | Specification artifacts | Validation artifacts |
| --- | --- | --- |
| 1 — Functional | [docs/specification.md](docs/specification.md) §1–2, [docs/roadmap.md](docs/roadmap.md) phase acceptance criteria | Hardware smoke tests, integration tests |
| 2 — Architecture | [README.md § Architecture](../README.md#architecture-overview), [docs/specification.md](docs/specification.md) §3–11 | Cross-compile CI, API contract tests |
| 3 — Module API | Public headers under `include/bossa/` | `tests/` mirroring `include/bossa/` (GTest) |
| 4 — Implementation | `src/`, `drivers/`, `server/` | Full CI + Pi 5 deployment smoke |

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

## Spec-driven development (SDD)

**Spec-driven development** treats written specifications—not code—as the
primary artifact. Code, tests, and design notes are derived from and validated
against those specs.

In BOSSA, the **project-level specification** lives in `docs/specification.md`
and `docs/roadmap.md`. Each **feature-level specification** is a GitHub issue
or PR description with concrete acceptance criteria.

### What a spec must contain

Before implementation starts, the spec for a change must be clear enough that an
independent reviewer (human or agent) could verify completion without guessing
intent:

| Element | Purpose | Where it lives |
| --- | --- | --- |
| **Intent** | Why the change exists | Issue title/body or PR summary |
| **Scope** | What is in and out of bounds | Issue or PR description |
| **Acceptance criteria** | Observable conditions of done | Issue checklist or PR test plan |
| **Traceability** | Link to `FR-*` / roadmap phase when applicable | Issue, PR, or test comments |
| **Constraints** | Non-negotiable rules (embedded, signal safety, no hot-path alloc) | This file + [docs/guidelines.md](docs/guidelines.md) |
| **Design notes** | Interfaces, modules, edge cases (when non-trivial) | PR description or `docs/specification.md` |

Write acceptance criteria in concrete, testable language (EARS-style: "When …,
the system shall …"). Ambiguous specs produce ambiguous code—refine the spec
before coding.

### SDD workflow in this repo

1. **Specify** — Capture intent, scope, and acceptance criteria in a GitHub
   issue. Map criteria to a [roadmap](docs/roadmap.md) phase and
   [specification](docs/specification.md) section when the change touches
   functional behavior.
2. **Plan** — For non-trivial work, identify which V-cycle levels are affected,
   which `docs/` files may need updates, and which test levels apply (unit /
   integration / hardware smoke).
3. **Task** — Break the plan into focused commits. Each commit addresses one
   logical step and traces back to at least one acceptance criterion.
4. **Implement and verify** — Follow the V-cycle at each level: stubs and tests
   before implementation where applicable; run quality gates locally before push.
5. **Review** — PR review checks that code matches the spec and that tests prove
   the acceptance criteria. **Humans merge; agents do not.**

### Rigor levels

| Level | When to use | Spec artifact |
| --- | --- | --- |
| **Spec-first** | Any merged change | Issue or PR with acceptance criteria |
| **Spec-anchored** | Public API, architecture, or `docs/` changes | Above + design notes and test plan |
| **Spec-as-source** | Large or AI-assisted features | Above + explicit task breakdown before coding |

Default for BOSSA: **spec-first** minimum; **spec-anchored** when module
boundaries, driver interfaces, or sync policies change.

### Rules

- Do not implement without a written spec (issue scope or PR description with
  acceptance criteria).
- When requirements change mid-task, update the spec first, then code and tests.
- Regressions: extend the spec with a failing test that encodes the bug, then fix.
- Do not add duplicate workflow documentation outside this file and
  [docs/guidelines.md](docs/guidelines.md).

Further reading:
[GitHub Spec Kit — Spec-Driven Development](https://github.com/github/spec-kit/blob/main/spec-driven.md),
[SDD and agentic AI for production-quality code](https://alexandrelheinen.pages.dev/articles/2026-04-22-ai-agents-sdd/)
(author's article on constraint layers and the SDD V-cycle).

---

## BOSSA development stages (4-level V-cycle)

BOSSA uses a **4-level V-cycle** as its software development lifecycle. Each
level has a descending artifact (specification or design) and an ascending
validation method. **Both sides of a level must be addressed before moving to the
next.**

An imperative order (implement, add, fix…) always implies the **full V-cycle**—not
just the code.

```
Level 1 — Functional specification  ◄──────────────►  Acceptance / hardware validation
  Level 2 — Architecture & interfaces  ◄──────────►  Cross-compile + API contract tests
    Level 3 — Module stubs & public API  ◄────────►  GTest unit tests (native x86_64)
      Level 4 — Implementation & private code  ◄──►  Full CI + Pi 5 smoke tests
```

For agile delivery, treat each GitHub issue as one complete V (the **W-cycle**:
chained small V-cycles). The next task starts from the verified output of the
previous one.

### Level 1 — Functional specifications

| Side | Artifact | BOSSA location |
| --- | --- | --- |
| **Specify** | Goals, sync policies, driver behavior, server API contract | [docs/specification.md](docs/specification.md), [docs/roadmap.md](docs/roadmap.md) phase criteria |
| **Verify** | Observable pass criteria on hardware or integration tests | Pi 5 smoke test, server + PostgreSQL end-to-end |

**Rule:** No Level 2 work on new behavior until the relevant specification
section and roadmap acceptance criteria exist.

**Validation:** Deploy to Pi 5; verify syslog output, SQLite rows, or PostgreSQL
inserts as appropriate.

### Level 2 — Architecture and interfaces

| Side | Artifact | BOSSA location |
| --- | --- | --- |
| **Specify** | Module decomposition, driver interface, sync engine, REST API | [README.md § Architecture](../README.md#architecture-overview), [docs/specification.md](docs/specification.md) §5–10 |
| **Verify** | Cross-compilation succeeds; API contract tests pass | `.github/workflows/build-and-test.yml` |

**Rule:** No Level 3 work on a new module boundary until
[docs/specification.md](docs/specification.md) defines the public interface.

### Level 3 — Module stubs and unit tests

| Side | Artifact | BOSSA location |
| --- | --- | --- |
| **Specify** | Public APIs: class declarations, Doxygen, stub bodies | `include/bossa/<module>/` |
| **Verify** | GTest unit tests written **with** the stubs | `tests/` (mirrors `include/bossa/`) |

Use `GTEST_SKIP()` or disabled tests with a documented reason for methods not yet
implemented. Enable tests when Level 4 fills the stub.

**CI gates:** `.github/workflows/build-and-test.yml` (native build + `ctest`).

**Rule:** Header + test files for a module land in the same commit. Hardware is
mocked—never require a Pi for unit tests.

### Level 4 — Implementation

| Side | Artifact | BOSSA location |
| --- | --- | --- |
| **Specify** | Algorithms, driver adapters, server wiring | `src/`, `drivers/`, `server/` |
| **Verify** | Full test suite, coverage ≥ 90%, Pi 5 smoke test | All CI workflows + hardware |

**Rule:** A feature is not done until CI is green and the relevant acceptance
criteria pass (unit tests on x86_64, cross-compile, and Pi smoke when I/O is
involved).

### SDD mapping to the V-cycle (GitHub workflow)

| V side | GitHub artifact | Agent / human action |
| --- | --- | --- |
| Left — Specify | **Issue** | What must be done, edge cases, tests that must pass (DoD) |
| Left — Design | **Issue + `docs/`** | Architecture notes, specification updates when needed |
| Right — Verify | **PR + CI** | GTest, formatting, native build, cross-compile |
| Right — Accept | **Human merge** | Review CI, hardware logs, PostgreSQL rows when applicable |

At the base of the V, the **agent loop** runs locally before each commit:
format → build → test → cross-compile → fix until green.

---

## Combining SDD, V-cycle, and TDD

SDD, the V-cycle, and test-driven development (TDD) operate at different layers.
Use them together in this order:

```
SDD (what & why)  →  V-cycle (structure each level)  →  TDD (build each unit)
     spec                 design ↔ tests                    red → green → refactor
```

| Step | Method | Activity |
| --- | --- | --- |
| 1 | **SDD** | Write intent, scope, acceptance criteria |
| 2 | **V-cycle** | Map criteria to test levels (hardware → integration → unit) |
| 3 | **V-cycle** | Update architecture / specification when boundaries change |
| 4 | **TDD** | At Level 3–4: failing GTest → minimal code → refactor |
| 5 | **V-cycle** | Run tests bottom-up; CI must pass |
| 6 | **SDD** | Update spec/docs if behavior changed; PR proves all criteria |

**TDD rules** (Kent Beck): do not write production code without a failing test;
eliminate duplication. TDD executes **detailed design** at Level 3–4—it does not
replace hardware smoke or integration specs from Level 1–2.

**Pause and re-spec** when acceptance criteria are ambiguous, scope grows beyond
the issue, or hardware tests reveal an unspecified requirement.

---

## Constraint layers for AI-assisted work

To get production-quality output from coding agents, BOSSA uses three constraint
layers (described in detail in the
[SDD article](https://alexandrelheinen.pages.dev/articles/2026-04-22-ai-agents-sdd/)).
Together they make hallucination visible and expensive to ignore.

### Layer 1 — Agent instructions (point to this file)

`AGENTS.md`, `CLAUDE.md`, and `.github/copilot-instructions.md` must **only**
direct agents to this document and [docs/guidelines.md](docs/guidelines.md).
Keep them short. Models have limited context—essential rules live here, not
scattered across ten files.

### Layer 2 — CI/CD quality gates

Automated checks enforce the method for humans and agents alike. No weakening
gates to make CI green. See [Quality gates](#quality-gates).

Agents must run local validation (`scripts/check/pre_push.sh` or equivalent
steps) before pushing.

### Layer 3 — Human merge

Every change is merged by a maintainer after reviewing CI results and, when
relevant, hardware evidence (syslog traces, PostgreSQL query results, oscilloscope
or logic-analyzer captures for timing-critical I/O). The agent does not decide
when work is done.

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

- GTest coverage target: **≥ 90%** on `bossa_core`, `bossa_telemetry`, `bossa_sync`,
  `bossa_server` (when those libraries exist)
- C++: Doxygen on public APIs, `clang-format` (LLVM style, C++20)
- Physical variables: `who_what` naming per [docs/guidelines.md](docs/guidelines.md)
- Embedded: no exceptions or heap allocation in hot paths (scheduler, `read()`,
  `write()`, signal handlers)

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
3. Open a PR against `main` with:
   - **Summary** — what changed and why (1–3 bullets).
   - **Test plan** — checklist of automated and manual verification.
   - **Traceability** — `FR-*` / roadmap phase references when applicable.
   - **Hardware plan** — Pi 5 smoke steps when I/O is involved.
4. Ensure CI is green before requesting review.
5. Address feedback in new commits (avoid force-push to `main`).

### Commit messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <imperative subject>

<body>

<footer>
```

Types: `feat`, `fix`, `docs`, `test`, `refactor`, `style`, `chore`, `build`, `ci`

Example:

```
feat(drivers): add BME280 I2C driver adapter

Implement bossa::drivers::Bme280Driver with mock I2C unit tests.
Validates FR-DRV-01 and Phase 2 acceptance criteria.

Tested on Raspberry Pi 5 with sensor at 0x76.

Closes #12
```

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

### Adding a server endpoint

1. Handler in `src/server/` or `include/bossa/server/`
2. Update [docs/specification.md](docs/specification.md) §10 REST API table
3. Contract test with in-process httplib client
4. PostgreSQL integration test via Docker Compose (when CI supports it)

### Adding a dependency

1. Add to `CMakeLists.txt` with `find_package()` or `FetchContent`
2. Add apt package to `scripts/setup.sh`
3. Document in [docs/specification.md](docs/specification.md) §4
4. Verify availability on Raspberry Pi OS Bookworm
5. Verify cross-compilation toolchain can link the library

### systemd service changes

1. Update unit file in `config/bossa.service` or `config/bossa-server.service`
2. Document in PR if `Type=`, `User=`, or `ExecStart=` changes
3. Test on Pi: `sudo systemctl daemon-reload && sudo systemctl restart bossa`

---

## Raspberry Pi 5 hardware validation

BOSSA targets **Raspberry Pi 5** running **Raspberry Pi OS Bookworm (64-bit)**.
Hardware validation is required when a change touches drivers, I/O, timing, or
deployment.

### Standard smoke procedure

```bash
# On development host
./scripts/build.sh -t toolchain-arm64.cmake
./scripts/sync.sh -t pi@raspberry.local

# On Pi (SSH)
sudo cp /opt/bossa/bossa.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl restart bossa
sudo journalctl -u bossa -n 30 --no-pager

# Foreground debug (no daemon fork)
sudo /opt/bossa/bin/bossa-daemon --foreground
```

### What to verify

| Change type | Evidence |
| --- | --- |
| Driver | Correct sample values in syslog; matches datasheet or known reference |
| Scheduler | Samples arrive at configured `sample_rate_hz` (±10%) |
| Sync / upload | Row appears in PostgreSQL `telemetry_points` table |
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
contributor.

### Context and intent

- Infer intent from the full conversation and issue scope, not only the latest message.
- Treat mid-task messages as refinements unless the user clearly changes direction.
- Read **this file** before making changes; it supersedes generic model habits.

### Scope and files

- Do not duplicate workflow rules in new markdown files.
- Do not edit unrelated files unless explicitly asked.
- An imperative order (implement, add, fix…) implies the **full V-cycle**, not code alone.

### Execution order

1. **SDD** — Confirm spec / acceptance criteria exist.
2. **V-cycle** — Identify level; update `docs/` when Level 1–2 change.
3. **Level 3** — Headers + GTest together when adding APIs. Mock all hardware.
4. **Level 4** — Implement; run `scripts/check/pre_push.sh` (or `--skip-cross`
   when iterating).
5. **PR** — Push only after `scripts/check/formatting.sh` passes; ensure CI
   green; document Pi smoke for I/O changes.

### Embedded-specific rules

- **No heap allocation** in `read()`, `write()`, scheduler loop, or ring buffer.
- **No exceptions** in hot paths or signal handlers.
- **Mock hardware** in unit tests—use `bossa::io` virtual interfaces.
- **syslog** for daemon logging, not `std::cout`.
- **Check system call return values**; log failures at `LOG_ERR`.
- **Cross-compile** every PR that changes production code.

### Git safety

- Never force-push to `main`.
- Never skip hooks unless explicitly requested.
- Never commit secrets (API keys, database passwords).
- Create commits when completing work that implies a PR.

### Communication

- Be precise; use code citations when referencing existing code.
- Keep responses proportional to task complexity.

---

## Reference documents

When an external guide conflicts with this file or [docs/guidelines.md](docs/guidelines.md),
**this repository wins**.

### BOSSA specifications

| Document | Role |
| --- | --- |
| [docs/specification.md](docs/specification.md) | Technical requirements, APIs, libraries, schemas |
| [docs/roadmap.md](docs/roadmap.md) | Phased delivery plan and acceptance criteria |
| [README.md § Architecture](../README.md#architecture-overview) | Level 2 — system design |
| [docs/guidelines.md](docs/guidelines.md) | C++ coding standards and formatting |

### Methodology (external)

| Topic | Resource |
| --- | --- |
| SDD | [GitHub Spec Kit — spec-driven.md](https://github.com/github/spec-kit/blob/main/spec-driven.md) |
| SDD + agents | [Author article — SDD and agentic AI](https://alexandrelheinen.pages.dev/articles/2026-04-22-ai-agents-sdd/) |
| V-model | [Teaching Agile — V-Model](https://teachingagile.com/sdlc/models/v-model) |
| TDD | [Kent Beck — Canon TDD](https://newsletter.kentbeck.com/p/canon-tdd) |
| FRET constitution | [FRET CONTRIBUTING.md](https://github.com/alexandrelheinen/fret/blob/main/CONTRIBUTING.md) |
| libgpiod | [libgpiod documentation](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/) |
| Embedded Linux | Love, R. — *Linux System Programming* |

---

## Pre-merge checklist

Every contributor (human or agent) must confirm before merge:

- [ ] Spec is clear: intent, scope, and acceptance criteria addressed.
- [ ] Work follows SDD → V-cycle → TDD at the appropriate levels.
- [ ] `FR-*` / roadmap traceability updated when behavior changed.
- [ ] `bash scripts/check/formatting.sh` passes.
- [ ] `./scripts/build.sh` passes (native x86_64).
- [ ] `ctest` passes when `tests/` is populated.
- [ ] `./scripts/build.sh -t toolchain-arm64.cmake` passes.
- [ ] CI green on the PR branch.
- [ ] Documentation updated when specs or public APIs changed.
- [ ] No quality gate weakened to pass.
- [ ] No secrets committed.
- [ ] Public APIs documented with Doxygen.
- [ ] Pi 5 smoke documented or executed when I/O changed.
- [ ] No exceptions or heap allocation introduced in hot paths.

If any item fails, fix forward—do not merge.

---

## License

By contributing, you agree that your contributions will be licensed under the
MIT License.
