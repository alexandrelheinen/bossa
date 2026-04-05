# bossa

<img src="docs/images/bossa.svg" alt="BOSSA Logo" width="200" align="left">

Bossa (Base Operating System for Sensors and Actuators) is a real-time system framework for IoT and robotics projects on ARM-based single-board computers (Raspberry Pi). It provides a production-ready foundation for building applications with hardware IO control, task priorities, telemetry, and logging capabilities.

The project emphasizes modern C++20 practices, testability, and rigorous V-cycle validation for embedded systems.

## Documentation Index

- [Coding guidelines (authoritative)](docs/guidelines.md)
- [Contributing guide](CONTRIBUTING.md)

## Continuous Integration

GitHub Actions runs the repository validation pipeline on pull request creation and updates.

The workflows are defined in `.github/workflows/` and enforce the checks described in `docs/guidelines.md`:

- **Formatting** (`formatting.yml`): C++ code formatting validation with clang-format
- **Build and Test** (`build-and-test.yml`):
  - Native build (x86_64) with unit tests (GTest)
  - Cross-compilation for ARM64 (Raspberry Pi 5)
  - Artifact generation for deployment

## Dependencies / Requirements

Install all build and deployment dependencies on a Debian-based system.

Usage:
```bash
./scripts/setup.sh
```

## Native Build

Usage:
```bash
./scripts/build.sh
```

## Cross-Compilation (Raspberry Pi 5 - ARM64)

For building on an x86_64 machine for Raspberry Pi 5:

Build for ARM64:
```bash
./scripts/build.sh -t toolchain-arm64.cmake
```

Output:
- Executable: `build/final/bin/bossa` (ARM64 binary)
- Archive: `build/final/bossa.tar.gz`

Available toolchain files:
- `toolchain-arm64.cmake` - For Raspberry Pi 5 (ARM64/aarch64)

### Sync (Installation)

Deploy the built artifacts to a remote Raspberry Pi device via SSH. Requires SSH access to the target device.

Usage:
```bash
./scripts/sync.sh [-t TARGET] [-d REMOTE_DIR]
```

Options:
- `-t TARGET`: Target device (default: `pi@raspberry.local`)
- `-d REMOTE_DIR`: Remote directory for deployment (default: `/opt/bossa`)
- `-h`: Show help message

Example:
```bash
./scripts/sync.sh -t pi@192.168.1.100 -d /opt/bossa
```

## Testing (native build)

After building it natively, you can test the service on your local Linux machine:

Build the service:
```bash
./scripts/build.sh
```

Run the service in the foreground (for debugging):
```bash
./build/final/bin/bossa
```

Run the service as a daemon:
```bash
sudo ./build/final/bin/bossa
```

The service will daemonize and run in the background. Monitor the logs with:
```bash
sudo tail -f /var/log/syslog | grep bossa
```

Or use journalctl:
```bash
sudo journalctl -u bossa -f
```

Stop the running service:
```bash
sudo pkill -SIGTERM bossa
```

Or send an interrupt signal:
```bash
sudo pkill -SIGINT bossa
```

### Install as a systemd service (optional)

Copy the service file:
```bash
sudo cp config/bossa.service /etc/systemd/system/
sudo systemctl daemon-reload
```

Start the service:
```bash
sudo systemctl start bossa
```

Check status:
```bash
sudo systemctl status bossa
```

Stop the service:
```bash
sudo systemctl stop bossa
```

View logs:
```bash
sudo journalctl -u bossa -f
```

## Code Formatting

The project uses **clang-format** to maintain consistent code style across C++ files. Code style is defined in `.clang-format` (LLVM style with C++20 standard).

### Prerequisites:

`clang-format` must be installed
```bash
sudo apt install clang-format
```

### Usage

Format all C++ files in the project using the provided script:
```bash
./scripts/clang.sh
```

The script automatically formats all `.cpp` and `.h` files in the `src/` and `include/` directories.

### IDE Integration (VS Code)

1. Install the C/C++ extension (ms-vscode.cpptools)
2. Install clang-format:
   ```bash
   sudo apt install clang-format
   ```
3. Configure VS Code settings:
   ```json
   "C_Cpp.clangFormatPath": "/usr/bin/clang-format",
   "editor.defaultFormatter": "ms-vscode.cpptools",
   "editor.formatOnSave": true
   ```
4. Format on save is now enabled

## CI and Merge Policy

GitHub Actions workflows run for pull requests and can be configured as required checks for merge protection on main.

Recommended required checks:

- **Formatting**: C++ formatting validation (formatting.yml)
- **Build and Test / Native Build**: x86_64 native build (build-and-test.yml)
- **Build and Test / Cross-Compile ARM64**: ARM64 cross-compilation (build-and-test.yml)

Unit tests will be automatically run when the `tests/` directory is populated.

## Contributing

Before contributing, follow [CONTRIBUTING.md](CONTRIBUTING.md) and the conventions in [docs/guidelines.md](docs/guidelines.md).

All contributions, including AI-assisted changes, must adhere to the V-cycle development process and pass all validation steps (native build, cross-compile, tests, format check).

## References

Core references for embedded C++ and Linux system programming:

- Stroustrup, B. (2013). The C++ Programming Language (4th Edition). Addison-Wesley.
- Meyers, S. (2014). Effective Modern C++. O'Reilly Media.
- Sutter, H., & Alexandrescu, A. (2004). C++ Coding Standards. Addison-Wesley.
- Love, R. (2013). Linux System Programming (2nd Edition). O'Reilly Media.
- ISO/IEC 14882:2020 - Programming Languages — C++
- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/

## License

MIT License. See [LICENSE](LICENSE).