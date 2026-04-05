# BOSSA Coding Guidelines

This document is the single authoritative reference for coding conventions in the BOSSA project. All contributors and AI agents should follow these rules rigorously — they are **not optional**.

BOSSA (Base Operating System for Sensors and Actuators) is a real-time embedded C++ framework for ARM-based IoT and robotics applications on Linux.

## 1. File and Package Structure

### Directory Layout

```
bossa/
├── include/bossa/         ← Public headers
├── src/                   ← Implementation files (.cpp)
├── tests/                 ← Unit tests (GTest)
├── config/                ← systemd service files, deployment configs
├── scripts/               ← Build, deployment, and utility scripts
├── docs/                  ← Documentation and design notes
└── CMakeLists.txt         ← Build configuration
```

### Header Organization

- **Public headers** live in `include/bossa/` and define the public API.
- **Private headers** (if needed) are co-located with their `.cpp` file in `src/`.
- Each header must have include guards or `#pragma once`.
- Headers should be self-contained (include all dependencies).

### Source Files

- **One class per file** when the class is substantial.
- Helper classes tightly coupled to a main class may coexist in the same file.
- File names match the primary class name in `snake_case`: `service.cpp`, `gpio_controller.cpp`.

### Namespace Convention

**Namespace mirrors directory structure**. All BOSSA code lives under the `bossa::` namespace.

Subdirectories create nested namespaces:
- `include/bossa/io/` → `namespace bossa::io`
- `include/bossa/sensors/` → `namespace bossa::sensors`

Example:
```cpp
// include/bossa/io/gpio.hpp
namespace bossa::io {
    class GPIO { /* ... */ };
}
```

## 2. Naming Conventions

Follow modern C++ and embedded systems conventions:

- **Classes and types**: `PascalCase` (e.g., `Service`, `GPIOController`, `I2CDevice`)
- **Functions and methods**: `snake_case` (e.g., `start()`, `read_sensor()`, `write_output()`)
- **Variables**: `snake_case` (e.g., `loop_count`, `sensor_value`)
- **Private/protected members**: `trailing_underscore_` (e.g., `name_`, `fd_`)
- **Constants**: `UPPER_CASE` (e.g., `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT_MS`)
- **Namespaces**: `snake_case` (e.g., `bossa::io`, `bossa::sensors`)
- **Macros**: `UPPER_CASE` with `BOSSA_` prefix (e.g., `BOSSA_VERSION`)

### Physical Variable Naming

All physical variables must follow the `who_what` convention.

**Physical quantity, not unit.** Never suffix a variable with its SI unit. Use the quantity name instead.

| Wrong | Correct |
|---|---|
| `timeout_ms` | `timeout_milliseconds` or `read_timeout` |
| `freq_hz` | `sample_frequency` |
| `temp_c` | `ambient_temperature` |
| `voltage_mv` | `supply_voltage` |

**Exception**: Configuration parameters in YAML/JSON or comments may use unit suffixes for human readability. The C++ variable name must still use the quantity.

**Qualifiers are prefixes.** `max`, `min`, `current`, `target` always come first.

| Wrong | Correct |
|---|---|
| `temperature_max` | `max_temperature` |
| `value_current` | `current_value` |

**Integers are suffixed with `_count`.**

| Wrong | Correct |
|---|---|
| `num_sensors` | `sensor_count` |
| `retry_attempts` | `retry_count` |

## 3. Documentation — Doxygen

All public APIs must be documented with Doxygen comments.

```cpp
/**
 * @brief Read a value from the specified GPIO pin.
 * 
 * @param pin The GPIO pin number to read.
 * @return true if the pin is HIGH, false if LOW.
 * @throws std::runtime_error if the pin is not configured for input.
 */
bool read_gpio(int pin);
```

Required tags:
- `@brief` — One-line summary
- `@param` — For each parameter
- `@return` — For non-void returns
- `@throws` — For intentional exceptions

Private implementation functions may have brief inline comments instead of full Doxygen blocks.

## 4. Code Formatting

All C++ files are formatted with **clang-format** using the `.clang-format` configuration at the repository root (LLVM style, C++20).

```bash
./scripts/clang.sh
```

- **Indentation**: 2 spaces (no tabs)
- **Line length**: 80 characters
- **Braces**: LLVM style (opening brace on same line for functions)
- **Pointers/references**: `Type* var` and `Type& var` (asterisk/ampersand with type)

Formatting is **mandatory** for production code (`src/`, `include/`).
Test files (`tests/`) should still be formatted but documentation can be lighter.

## 5. Modern C++ Practices

BOSSA uses **C++20**. Follow modern C++ idioms:

### Memory Management

- **Prefer RAII**: Use smart pointers and scope-based resource management.
- **No raw `new`/`delete`**: Use `std::make_unique`, `std::make_shared`.
- **Smart pointers**:
  - `std::unique_ptr` for single ownership
  - `std::shared_ptr` for shared ownership (use sparingly)
  - `std::weak_ptr` to break circular references
- **No naked pointers** except for non-owning references or C API interop.

### Type Safety

- Prefer `auto` for type deduction when the type is obvious from context.
- Use `const` liberally for read-only parameters and members.
- Use `constexpr` for compile-time constants.
- Prefer `enum class` over plain `enum`.
- Use `std::optional<T>` for nullable values instead of pointers.

### Error Handling

- **Exceptions are allowed** in initialization and non-critical paths.
- **No exceptions in real-time loops** or signal handlers.
  - Use return codes or `std::expected<T, E>` (C++23) for critical paths.
- **System call errors**: Check return values and log appropriately via syslog.

### Concurrency

- Use `std::thread`, `std::mutex`, `std::condition_variable` for threading.
- Prefer `std::atomic` for lock-free primitives.
- **Signal safety**: Code in signal handlers must be async-signal-safe (no malloc, no mutex, no syslog).
  - Use `volatile sig_atomic_t` for signal flags.

### Standard Library

- Prefer STL containers over custom implementations.
- Use `std::chrono` for time measurements and durations.
- Use `std::filesystem` for file operations.

## 6. Testing

### V-Cycle Mandate

Tests must be defined **at the same time** as the code they cover (V-cycle). They should be created before moving on to the next task.

An imperative order (do, implement, make, add...) is not only about writing the code. It must include all the V-cycle.

### Respect the V-cycle

All work must include all the descending and ascending steps of the cycle. For each row numbered below, the two actions (descend and ascend) must be done **at the same time**:

1. **Documentation and acceptance criteria**: Document the work/feature in GitHub issues or PR comments. Include goals, objectives, and acceptance criteria that can be verified.

2. **Architecture and functional tests**: Implement the architecture (classes, public interface, file organization, dependencies) and functional unit tests **at the same time**. Testing must come first: the performance of the algorithm is independent of its implementation. By reading the acceptance criteria, you must already know which values to expect.

3. **Implementation and fine testing**: Fill the stubs from the architecture definition. Implement algorithms, data structures, and private utilities. Add unit testing for private functions (fine testing).

4. **Test execution and validation**: Run the tests aiming for 100% coverage (at least 90%). If this step fails, go back to step 2: review architecture and functional tests.

5. **Integration and documentation**: If all tests pass:
   - Test on target hardware (cross-compile and deploy to Raspberry Pi if possible)
   - Update documentation for the newly implemented feature
   - Ensure all workflows pass (build, format checks, tests)
   - If something is wrong, go back to step 1

### C++ Testing Framework

- Use **Google Test (GTest)** for unit tests.
- Place tests in `tests/` mirroring the `src/` and `include/` structure.
- Test executables are built by CMake and run via `ctest`.

Example test structure:
```cpp
#include <gtest/gtest.h>
#include "bossa/service.hpp"

TEST(ServiceTest, ConstructorSetsName) {
    // Test implementation
}
```

### Testing Strategy

1. **Unit tests**: Test individual classes and functions in isolation (use mocks for hardware).
2. **Integration tests**: Test interactions between components (may require real hardware or simulators).
3. **Hardware tests**: Deploy to Raspberry Pi and run smoke tests.

### Cross-Platform Testing

- **Native build** (x86_64): Run unit tests locally during development.
- **Cross-compiled** (ARM64): Deploy to target hardware for integration testing.
- Use QEMU for ARM64 emulation when hardware is unavailable (limited usefulness for I/O testing).

## 7. Embedded and Real-Time Considerations

### Resource Constraints

- **Memory**: Raspberry Pi 5 has ample RAM, but avoid excessive allocations in loops.
- **CPU**: Avoid busy-waiting; use sleep, condition variables, or interrupts.
- **I/O bandwidth**: Cache sensor readings when appropriate; don't poll unnecessarily.

### Real-Time Guidelines

- **Minimize allocations** in hot paths (pre-allocate buffers).
- **Avoid blocking** in time-critical loops.
- **No exceptions** in real-time sections (catch at loop boundaries).
- **Signal handlers**: Keep minimal and async-signal-safe.

### Hardware Abstraction

- Abstract hardware interfaces behind clean APIs.
- Use dependency injection or factory patterns for testability.
- Example:
  ```cpp
  class IGPIOController {
  public:
      virtual ~IGPIOController() = default;
      virtual bool read_pin(int pin) = 0;
      virtual void write_pin(int pin, bool value) = 0;
  };
  ```

### Linux System Programming

- **Signal handling**: Use `sigaction()` over `signal()` for portability.
- **Daemon conventions**: Follow the daemonization pattern in `Service` class.
- **Logging**: Use `syslog()` for daemon logs; avoid `std::cout` in production daemons.
- **File descriptors**: Always check return values; close FDs in destructors or RAII wrappers.

## 8. Build and Dependency Management

### CMakeLists.txt

- Use modern CMake (3.16+).
- Set compile features: `set(CMAKE_CXX_STANDARD 20)`
- Organize targets clearly:
  ```cmake
  add_library(bossa_core STATIC ${CORE_SOURCES})
  add_executable(bossa src/main.cpp)
  target_link_libraries(bossa PRIVATE bossa_core)
  ```
- Install binaries and headers for deployment.

### Dependencies

- **Minimize external dependencies** for embedded targets.
- Prefer header-only libraries or system libraries available via apt.
- Document any non-standard dependencies in README.
- Use `find_package()` for third-party libraries.

### Cross-Compilation

- Use CMake toolchain files (e.g., `toolchain-arm64.cmake`).
- Test toolchain files regularly to avoid drift.
- Cross-compiled binaries go to `build/final/bin/bossa`.

## 9. systemd Integration

### Service Files

- Service files live in `config/` (e.g., `bossa.service`).
- Follow systemd best practices:
  - Use `Type=forking` if service daemonizes itself.
  - Use `Type=simple` or `Type=notify` for non-daemonizing services.
  - Include `Restart=on-failure` for robustness.

### Logging

- Use `syslog()` for all daemon logging.
- Log levels:
  - `LOG_DEBUG`: Verbose diagnostics (disabled in production).
  - `LOG_INFO`: Normal operation events.
  - `LOG_WARNING`: Recoverable issues.
  - `LOG_ERR`: Errors requiring attention.
  - `LOG_CRIT`: Critical failures (service may terminate).

### Deployment

- Use `scripts/sync.sh` to deploy to remote Raspberry Pi devices.
- Deployment includes:
  - Binary (`bossa`)
  - Service file (`bossa.service`)
  - Configuration files (if any)

## 10. Language — US English Only

All source code identifiers, comments, documentation (Markdown), configuration files, and commit messages **must use American English** spelling. This rule applies to all human contributors and AI agents without exception.

Common corrections:

| Use | Not |
|-----|-----|
| color | colour |
| behavior | behaviour |
| initialize / initialized | initialise / initialised |
| optimize / optimized / optimizer | optimise / optimised / optimiser |
| center / centered | centre / centred |

**Exception:** External library API parameters that use UK spelling must be left unchanged to avoid breaking calls.

## 11. Pre-flight Checklist

Before finishing **any** implementation task, all of the following must pass locally. This applies to both human contributors and AI agents.

```bash
# 1. Native build (x86_64)
./scripts/build.sh

# 2. Run unit tests (if available)
cd build && ctest -V

# 3. Code formatting — zero issues
./scripts/clang.sh
git diff --exit-code  # Verify no formatting changes needed

# 4. Cross-compile for ARM64
./scripts/build.sh -t toolchain-arm64.cmake

# 5. (Optional) Deploy and smoke test on Raspberry Pi
./scripts/sync.sh -t pi@raspberry.local
ssh pi@raspberry.local 'sudo systemctl restart bossa && sleep 2 && sudo systemctl status bossa'
```

All GitHub workflow checks (build, format, tests) must pass before opening or merging a pull request.

## 12. Git and Version Control

### Commit Messages

Follow conventional commits format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `build`

Example:
```
feat(io): add GPIO controller with interrupt support

Implement GPIOController class with edge-triggered interrupts
using Linux sysfs GPIO interface. Supports rising, falling, and
both edge triggers.

Closes #15
```

### Branch Strategy

- `main` is protected; all changes via pull requests.
- Feature branches: `feature/<name>`
- Bugfix branches: `fix/<name>`
- Keep branches focused and short-lived.

### Pull Requests

- Reference related issues.
- Provide clear description of changes.
- Ensure all CI checks pass (build, format, tests).
- Test on target hardware when possible before merging.

---

These guidelines are mandatory for all development in BOSSA. When in doubt, refer to this document first, then modern C++ best practices, then Linux system programming conventions.
