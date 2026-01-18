# bossa

<img src="images/bossa_logo.png" alt="BOSSA Logo" width="200" align="left">

Bossa (Base Operating System for Sensors and Actuators) is a real-time system framework for IoT and robotics projects on ARM-based single-board computers (Raspberry Pi). It provides a production-ready foundation for building applications with hardware IO control, task priorities, telemetry, and logging capabilities.

<br clear="all">

## Dependencies / Requirements

Install all build and deployment dependencies on a Debian-based system.

Usage:
```bash
./scripts/setup.sh
```

## Build

Compile the project using CMake and create a distributable `tar.gz` archive.

### Native Build

Usage:
```bash
./scripts/build.sh
```

### Cross-Compilation (Raspberry Pi 5 - ARM64)

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

## Sync (Installation)

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