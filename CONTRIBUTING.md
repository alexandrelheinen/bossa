# Contributing to BOSSA

Thank you for contributing to the Base Operating System for Sensors and Actuators project.

## Prerequisites

- Ubuntu 20.04+ or Debian-based Linux distribution
- GCC 10+ with C++20 support
- CMake 3.16+
- Git
- For cross-compilation: ARM64 cross-compiler toolchain
- For deployment: SSH access to Raspberry Pi device

## Setup

```bash
git clone https://github.com/alexandrelheinen/bossa.git
cd bossa
./scripts/setup.sh
```

## Development Workflow

1. Create a feature branch from main.
2. Implement the change with focused commits following the V-cycle.
3. Add or update tests (GTest unit tests required).
4. Run local validation before opening a pull request.
5. Ensure all CI checks pass.

## Local Validation

### Native Build (x86_64)

Build for your local machine:

```bash
./scripts/build.sh
```

### Run Unit Tests

```bash
cd build
ctest -V
```

### Format Code

```bash
./scripts/clang.sh

# Verify no changes needed
git diff --exit-code
```

### Cross-Compile for ARM64

Build for Raspberry Pi 5:

```bash
./scripts/build.sh -t toolchain-arm64.cmake
```

Output will be in `build/final/bin/bossa` (ARM64 binary).

### Deploy and Test on Hardware

Deploy to Raspberry Pi:

```bash
./scripts/sync.sh -t pi@raspberry.local -d /opt/bossa
```

SSH into the device and test:

```bash
ssh pi@raspberry.local

# Run in foreground (for debugging)
sudo /opt/bossa/bin/bossa

# Or install as systemd service
sudo cp /opt/bossa/bossa.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl start bossa
sudo systemctl status bossa

# View logs
sudo journalctl -u bossa -f
```

## Coding Rules

The authoritative coding standard is [docs/guidelines.md](docs/guidelines.md).

All contributions, including AI-assisted changes, must follow this file.

### Key Conventions

- **C++20**: Modern C++ with RAII, smart pointers, no raw new/delete
- **Formatting**: `clang-format` with LLVM style (enforced by CI)
- **Documentation**: Doxygen comments for all public APIs
- **Testing**: V-cycle development, GTest unit tests required, aim for 90%+ coverage
- **Physical variables**: `who_what` naming (e.g., `max_sample_rate`, not `sample_rate_max`)
- **Real-time**: No exceptions or allocations in hot paths
- **Signal safety**: Signal handlers must be async-signal-safe

## V-Cycle Development

BOSSA follows a rigorous V-cycle development process. Every task must include:

1. **Requirements**: Document goals and acceptance criteria (GitHub issue or PR description)
2. **Architecture + Functional Tests**: Design interfaces and write tests **before** implementation
3. **Implementation + Fine Tests**: Fill stubs and add private function tests
4. **Validation**: Run tests (target 100% coverage, minimum 90%)
5. **Integration**: Test on hardware, update documentation, ensure CI passes

**Never** submit code without corresponding tests. See [docs/guidelines.md](docs/guidelines.md) section 6 for full details.

## Pull Request Checklist

Before opening a PR, verify:

- [ ] Behavior changes are covered by unit tests
- [ ] Public APIs are documented (Doxygen comments)
- [ ] Code is formatted (`./scripts/clang.sh` with no changes)
- [ ] Native build passes (`./scripts/build.sh`)
- [ ] Unit tests pass (`cd build && ctest -V`)
- [ ] Cross-compilation succeeds (`./scripts/build.sh -t toolchain-arm64.cmake`)
- [ ] (If possible) Tested on Raspberry Pi hardware
- [ ] Documentation updated (README, guidelines, or docs/ as appropriate)
- [ ] Commit messages follow conventional commits format
- [ ] Branch is up-to-date with main

## Documentation

Update documentation when adding or changing behavior:

- [README.md](README.md) for user-facing changes (installation, usage, deployment)
- [docs/guidelines.md](docs/guidelines.md) for new coding rules or conventions
- Inline code documentation (Doxygen) for all public APIs
- systemd service file (`config/bossa.service`) if changing daemon behavior

## Adding Components

### Adding a New Hardware Interface

1. Create header in `include/bossa/<domain>/<component>.hpp`
2. Implement in `src/<component>.cpp`
3. Add unit tests in `tests/<component>_test.cpp` (use mocks for hardware)
4. Update CMakeLists.txt to include new sources
5. Document the interface with Doxygen comments
6. Test on actual hardware when possible

Example structure:
```cpp
// include/bossa/io/gpio_controller.hpp
namespace bossa::io {
    class GPIOController {
    public:
        virtual ~GPIOController() = default;
        virtual bool read_pin(int pin) = 0;
        virtual void write_pin(int pin, bool value) = 0;
    };
}
```

### Adding a Service

1. Inherit from `bossa::Service` base class
2. Implement the `loop()` method
3. Add unit tests for business logic (mock Service base class if needed)
4. Update CMakeLists.txt to build the new executable
5. Create corresponding systemd service file in `config/`

### Adding Dependencies

1. Add to CMakeLists.txt with `find_package()` or `add_subdirectory()`
2. Document in README Prerequisites section
3. Ensure dependency is available via apt on Raspberry Pi OS
4. Ensure cross-compilation toolchain can find the dependency

## Testing Strategy

### Unit Tests (GTest)

- **Required** for all new code
- Test individual classes and functions in isolation
- Use mocks for hardware interfaces (GPIO, I2C, SPI)
- Run on native x86_64 during development (fast iteration)

Example:
```cpp
#include <gtest/gtest.h>
#include "bossa/service.hpp"

class MockService : public bossa::Service {
public:
    MockService() : Service("test") {}
    void loop() override { /* test implementation */ }
};

TEST(ServiceTest, ConstructorSetsName) {
    MockService service;
    // Assertions
}
```

### Integration Tests

- Test component interactions
- May require real hardware or hardware simulators
- Deploy to Raspberry Pi for realistic testing

### Hardware Tests

- Deploy to Raspberry Pi using `./scripts/sync.sh`
- Run smoke tests: start service, check logs, verify behavior
- Test under realistic conditions (timing, load, I/O)

## Cross-Compilation Workflow

BOSSA supports cross-compilation for ARM64 (Raspberry Pi 5):

1. **Toolchain**: Uses `toolchain-arm64.cmake` to configure cross-compiler
2. **Build**: `./scripts/build.sh -t toolchain-arm64.cmake`
3. **Output**: ARM64 binary in `build/final/bin/bossa`
4. **Archive**: `build/final/bossa.tar.gz` for easy deployment
5. **Deploy**: `./scripts/sync.sh -t pi@raspberry.local`

### Testing Cross-Compiled Code

- **QEMU** can emulate ARM64 for basic validation (limited for I/O)
- **Real hardware** is required for GPIO, I2C, SPI, and timing-sensitive code

## Commit Messages

Follow conventional commits format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `build`

**Example**:
```
feat(io): add GPIO controller with interrupt support

Implement GPIOController class with edge-triggered interrupts
using Linux sysfs GPIO interface. Supports rising, falling, and
both edge triggers. Tested on Raspberry Pi 5.

Closes #15
```

## Getting Help

- Review existing code and tests for examples
- Check [docs/guidelines.md](docs/guidelines.md) for detailed conventions
- Open a GitHub issue for questions or clarifications
- For embedded Linux questions, consult Linux kernel GPIO/I2C documentation

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
