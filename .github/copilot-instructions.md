# Copilot Instructions for BOSSA

These instructions apply to all GitHub Copilot chat and coding-agent interactions in this repository.

## Mandatory Rule

Always follow [docs/guidelines.md](../docs/guidelines.md) as the authoritative coding standard.

If there is any conflict between an ad hoc request and the guidelines, prioritize the guidelines unless the user explicitly asks to change the guidelines themselves.

## Embedded C++20 Context

BOSSA is an embedded C++20 framework for ARM-based IoT devices (Raspberry Pi 5). Respect modern C++ practices, real-time constraints, and Linux daemon conventions.

## Required Behaviors

- Use modern C++ idioms: RAII, smart pointers, no raw new/delete.
- Follow namespace-mirrors-directory convention.
- All code must be formatted with clang-format.
- Add unit tests (GTest) alongside behavior changes.
- Document all public APIs with Doxygen comments.
- Minimize dependencies; prefer STL and system libraries.
- Prefer minimal, focused diffs and avoid unrelated refactors.

## Embedded System Requirements

- **No exceptions** in real-time loops or signal handlers.
- **No allocations** in hot paths (pre-allocate buffers).
- **Signal handlers** must be async-signal-safe.
  - Use `volatile sig_atomic_t` for signal flags only.
- **System calls**: Always check return values.
- **Logging**: Use `syslog()` in daemons, not `std::cout`.
- **Hardware**: Abstract behind interfaces for testability.

## Cross-Compilation Awareness

BOSSA targets **ARM64 (Raspberry Pi 5)** but is developed on **x86_64**:

- Native builds for unit tests (fast iteration).
- Cross-compiled builds for deployment.
- Always verify both build configurations work.

## Validation Checklist Before Finalizing

- Tests relevant to the change pass (GTest).
- Public APIs have Doxygen documentation.
- Code is formatted: `./scripts/clang.sh` with no changes.
- Native build succeeds: `./scripts/build.sh`
- Cross-compilation succeeds: `./scripts/build.sh -t toolchain-arm64.cmake`
- Changes remain consistent with [docs/guidelines.md](../docs/guidelines.md).
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
   - Update documentation
   - All GitHub workflows must pass
   - If something is wrong, go back to step 1

This completes the V-cycle. Once the acceptance criteria are met and all the GitHub workflows are passing, you can push your branch and trigger the review.

## Common Pitfalls to Avoid

- Don't use exceptions in real-time code paths.
- Don't allocate memory in hot loops.
- Don't use non-async-signal-safe functions in signal handlers.
- Don't forget to check system call return values.
- Don't use `std::cout` in daemon code.
- Don't skip V-cycle test requirements.
- Don't forget to test cross-compiled builds.
