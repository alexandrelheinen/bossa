# CLAUDE Instructions

This repository uses this file as persistent guidance for Claude-based coding agents.

## Mandatory Standard

Use [docs/guidelines.md](docs/guidelines.md) as the required coding and architecture reference for all changes.

BOSSA is a C++20 embedded framework for ARM-based IoT devices. Respect modern C++ practices, real-time constraints, and Linux system programming conventions.

## Required Practices

- Follow modern C++ idioms: RAII, smart pointers, no raw new/delete.
- Respect the namespace-mirrors-directory convention for C++ code.
- All code must be formatted with clang-format before commit.
- Add unit tests (GTest) with behavior changes.
- Document all public APIs with Doxygen comments.
- Use explicit typing and `const` correctness.
- Keep changes small, composable, and aligned with existing style.
- Minimize dependencies; prefer STL and system libraries.

## Embedded and Real-Time Requirements

- **No exceptions** in real-time loops or signal handlers.
- **No allocations** in hot paths (pre-allocate buffers).
- **Signal handlers** must be async-signal-safe (no malloc, no mutex, no syslog).
  - Use `volatile sig_atomic_t` for signal flags.
- **System calls**: Always check return values and handle errors appropriately.
- **Logging**: Use `syslog()` for daemons, not `std::cout`.
- **Hardware abstractions**: Use interfaces for testability (mock GPIO, I2C, etc.).

## Cross-Compilation Context

BOSSA targets **Raspberry Pi 5 (ARM64)** but is developed on **x86_64**.

- **Native builds** (x86_64): For unit tests and fast iteration.
- **Cross-compiled builds** (ARM64): For deployment to Raspberry Pi.
- **Testing strategy**: Unit tests run natively; integration tests run on hardware.
- Always ensure changes work in both native and cross-compiled contexts.

## Delivery Quality

Before finishing a task, verify:

- Relevant tests pass (GTest unit tests).
- Public APIs are documented with Doxygen.
- Code is formatted: `./scripts/clang.sh` with no changes.
- Native build succeeds: `./scripts/build.sh`
- Cross-compilation succeeds: `./scripts/build.sh -t toolchain-arm64.cmake`
- File organization complies with [docs/guidelines.md](docs/guidelines.md).
- An imperative order (do, implement, make, add...) is not only about writing the code. It must include all the V-cycle.

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

## systemd and Daemon Workflow

When making changes that affect daemon behavior:

1. **Build**: Native and cross-compile
2. **Deploy**: Use `./scripts/sync.sh -t pi@raspberry.local`
3. **Test**: SSH to device, restart service, check logs
4. **Validate**: Verify expected behavior on hardware

## Common Pitfalls to Avoid

- Don't use exceptions in real-time code paths.
- Don't allocate memory in hot loops (pre-allocate).
- Don't use non-async-signal-safe functions in signal handlers.
- Don't forget to check system call return values.
- Don't use `std::cout` in daemon code (use syslog).
- Don't skip the V-cycle steps; all code must have tests.
- Don't mix interface definitions with hardware-specific implementations.
- Don't forget to test both native and cross-compiled builds.
