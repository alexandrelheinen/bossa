# CMake Toolchain File for ARM64 (Raspberry Pi 5)
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm64.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Specify the cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Specify the cross linker
set(CMAKE_AR aarch64-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-linux-gnu-ranlib)

# Flags for cross compilation
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")

# Multiarch sysroot for aarch64 Debian/Ubuntu packages (sqlite, curl, …).
set(CMAKE_FIND_ROOT_PATH
    /usr/aarch64-linux-gnu
    /usr/lib/aarch64-linux-gnu
    ${CMAKE_FIND_ROOT_PATH})
list(APPEND CMAKE_PREFIX_PATH /usr/lib/aarch64-linux-gnu)
set(ENV{PKG_CONFIG_LIBDIR}
    "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig")

# Search for programs only in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# For libraries and headers in the target directories.
# INCLUDE=BOTH: multiarch headers such as sqlite3.h live under /usr/include.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# Hint multiarch library locations for find_package(SQLite3/CURL).
set(SQLite3_INCLUDE_DIR /usr/include CACHE PATH "" FORCE)
set(SQLite3_LIBRARY /usr/lib/aarch64-linux-gnu/libsqlite3.so CACHE FILEPATH "" FORCE)
set(CURL_INCLUDE_DIR /usr/include/aarch64-linux-gnu CACHE PATH "" FORCE)
set(CURL_LIBRARY /usr/lib/aarch64-linux-gnu/libcurl.so CACHE FILEPATH "" FORCE)

# libgpiod is linked only on native builds (see CMakeLists.txt). Cross-compiled
# binaries use the GPIO stub backend; I2C and drivers still work on the Pi.
