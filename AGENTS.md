# AGENTS Instructions

Automated agents working in this repository must follow
[CONTRIBUTING.md](CONTRIBUTING.md) as the **single source of truth** for
development workflow, SDD, the 4-level V-cycle, quality gates, and agent policy.

For coding conventions (C++20, embedded constraints, Doxygen, naming), follow
[docs/guidelines.md](docs/guidelines.md).

**Do not duplicate** workflow or V-cycle rules in this file. When instructions
conflict, resolve in this order:

1. Direct maintainer request in the active task
2. [CONTRIBUTING.md](CONTRIBUTING.md)
3. [docs/guidelines.md](docs/guidelines.md)
4. [docs/specification.md](docs/specification.md)
5. Modern C++ best practices (C++ Core Guidelines)

An imperative order (implement, add, fix…) always implies the full V-cycle
described in [CONTRIBUTING.md](CONTRIBUTING.md), not code alone.

## Pre-push gates (mandatory)

Before every `git push` on a PR branch, agents **must** run at minimum:

```bash
bash scripts/check/formatting.sh
./scripts/build.sh
```

Do not push or update a PR until both pass. When time allows, prefer the full
local gate before push:

```bash
bash scripts/check/pre_push.sh
```

Pushing with formatting or build failures is unacceptable.

## Cursor Cloud specific instructions

Notes for cloud agents working in this repository:

- **Dual-target builds:** BOSSA is developed on x86_64 and deployed on ARM64
  (Raspberry Pi 5). Always verify the native build; run cross-compile via
  `bash scripts/check/pre_push.sh` or
  `./scripts/build.sh -t toolchain-arm64.cmake` before claiming done.
- **No Pi attached:** Cloud environments do not have a Raspberry Pi 5 connected.
  Run GTest with mocked `bossa::io` interfaces. Document the Pi 5 smoke
  procedure in the PR test plan for human execution—never claim hardware
  validation without evidence.
- **Dependencies:** Run `./scripts/setup.sh` if the compiler or toolchain is
  missing. The setup script installs build-essential, cmake, clang-format, and
  the ARM64 cross-compiler.
- **Embedded constraints:** No heap allocation or exceptions in hot paths
  (driver `read()`/`write()`, scheduler, ring buffer, signal handlers). Use
  `syslog()` for daemon logging.
- **Specification stack:** Read [docs/specification.md](docs/specification.md)
  for APIs and library choices; [docs/roadmap.md](docs/roadmap.md) for the
  current phase and acceptance criteria.
- **Ecosystem:** BOSSA syncs telemetry to PostgreSQL for the companion Freshy
  SQL project (private repo). Edge uploads via HTTPS; the server writes to SQL.
