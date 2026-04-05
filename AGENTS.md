# AGENTS Instructions

This file defines repository-wide instructions for coding agents.

## Scope

Applies to all automated agents working in this repository.

## Source of Truth

Follow [docs/guidelines.md](docs/guidelines.md) as the authoritative standard for:

- C++20 modern practices and embedded conventions
- File and namespace organization
- Doxygen documentation style
- Code formatting (clang-format)
- Testing requirements and V-cycle methodology
- Real-time and embedded system constraints
- Cross-compilation workflow

## Agent Policy

- Do not introduce patterns that violate [docs/guidelines.md](docs/guidelines.md).
- Keep edits narrow and relevant to the task.
- When changing behavior, update tests in the mirrored tests structure.
- When adding public APIs, include Doxygen documentation.
- Prefer project-local conventions over generic defaults.
- Respect embedded system constraints (memory, real-time, signal safety).

An imperative order (do, implement, make, add...) is not only about writing the code. It must include all the V-cycle.

### Respect the V-cycle

As indicated above, all work of an AI must include all the descending and ascending steps of the cycle. For each row numbered below, the two actions (descend and ascend) must be done **at the same time**:

1. **Add documentation of the work, feature, bug fix**: Use GitHub issues if possible, otherwise go directly into the GitHub PR and document every step in comments. This includes: goal/objectives and acceptance criteria.

2. **Implement the architecture** of the code (classes, public interface, file organization, dependencies) and the **(functional) unit tests at the same time**. The testing must come first then coding: the performance of the algorithm is independent of its implementation. By reading the acceptance criteria above, you must already know which values to expect.

3. **Do the coding**: This is the 3rd step: Fill the stubs left by the architecture definition. Implement algorithms, data structures, and private/local utilities. Add unit testing for private functions as well (fine testing/non-functional tests).

4. **Then, run the tests**: Ideally proving 100% coverage (at least 90% would be great!). If this step fails, go back to step 2: Review your architecture, your functional tests, and go back to the cycle.

5. **Implement integration validation** if all unit tests are passing:
   - Cross-compile for ARM64
   - Deploy to Raspberry Pi (if available)
   - Run smoke tests on hardware
   - Update documentation for the newly implemented feature
   - All GitHub workflows must pass
   - If something is wrong, go back to step 1

This completes the V-cycle. Once the acceptance criteria are met and all the GitHub workflows are passing, you can push your branch and trigger the review.

## Embedded C++ Specific Requirements

When working with embedded C++ components:

- **Memory management**: Use RAII and smart pointers exclusively (no raw new/delete).
- **Real-time paths**: No exceptions, no allocations, no blocking calls.
- **Signal handlers**: Must be async-signal-safe (use `volatile sig_atomic_t` flags only).
- **Hardware abstraction**: Define interfaces for testability; mock hardware in unit tests.
- **System calls**: Always check return values and handle errors.
- **Logging**: Use `syslog()` in daemons, not `std::cout`.

## Cross-Compilation Requirements

BOSSA targets ARM64 (Raspberry Pi 5) but is developed on x86_64:

- **Dual build verification**: Test both native and cross-compiled builds.
- **Unit tests**: Run on native x86_64 for fast iteration.
- **Integration tests**: Deploy to Raspberry Pi for hardware validation.
- **Toolchain**: Use `toolchain-arm64.cmake` for cross-compilation.

## Pre-commit Validation

Before completing any task, run the pre-flight checklist from [docs/guidelines.md](docs/guidelines.md):

1. Native build: `./scripts/build.sh`
2. Run unit tests: `cd build && ctest -V`
3. Format check: `./scripts/clang.sh && git diff --exit-code`
4. Cross-compile: `./scripts/build.sh -t toolchain-arm64.cmake`
5. (Optional) Deploy and test: `./scripts/sync.sh -t pi@raspberry.local`

## Conflict Resolution

When instructions conflict, resolve in this order:

1. Direct maintainer request in the active task
2. [docs/guidelines.md](docs/guidelines.md)
3. Modern C++ best practices (C++ Core Guidelines)
4. Linux system programming conventions
5. This file
