# bossa

<img src="docs/images/bossa.svg" alt="BOSSA Logo" width="200" align="left">

Bossa (Base Operating System for Sensors and Actuators) is a real-time system framework for IoT and robotics projects on ARM-based single-board computers (Raspberry Pi). It provides a production-ready foundation for building applications with hardware IO control, task priorities, telemetry, and logging capabilities.

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