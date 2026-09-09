# AGENTS Instructions

This file is a bridge only. **Do not add rules here.**

Shared engineering guidelines live in [.guidelines/](.guidelines/) (a git
submodule):

- [.guidelines/workflow/sdd.md](.guidelines/workflow/sdd.md), [integration.md](.guidelines/workflow/integration.md), [tdd.md](.guidelines/workflow/tdd.md) — how work gets done
- [.guidelines/agents/writing.md](.guidelines/agents/writing.md) — how any prose should read
- [.guidelines/style/naming.md](.guidelines/style/naming.md) — naming
- [.guidelines/languages/cpp.md](.guidelines/languages/cpp.md), [cmake.md](.guidelines/languages/cmake.md), [sh.md](.guidelines/languages/sh.md) — C++, CMake, shell

For BOSSA's own project context, ecosystem, embedded/hardware policy, and
merge policy, read [CONTRIBUTING.md](CONTRIBUTING.md). For BOSSA-specific
coding notes, read [docs/guidelines.md](docs/guidelines.md).

Conflict order: direct maintainer request > CONTRIBUTING.md > `.guidelines/`
> `docs/guidelines.md` > `docs/specification.md` > modern C++ best
practices (C++ Core Guidelines).

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

## Cursor Cloud specific instructions

- **Dual-target builds:** BOSSA is developed on x86_64 and deployed on
  ARM64 (Raspberry Pi 5). Always verify the native build; run
  cross-compile via `bash scripts/check/pre_push.sh` or
  `./scripts/build.sh -t toolchain-arm64.cmake` before claiming done.
- **No Pi attached:** cloud environments do not have a Raspberry Pi 5
  connected. Run GTest with mocked `bossa::io` interfaces. Document the
  Pi 5 smoke procedure in the PR test plan for human execution, never
  claim hardware validation without evidence.
- **Dependencies:** run `./scripts/setup.sh` if the compiler or toolchain
  is missing.
- **Ecosystem:** BOSSA syncs telemetry to a BOSSA-owned Cloudflare Worker
  + D1 (SQLite). Edge keeps a local SQLite offline buffer; uploads go
  over HTTPS.
