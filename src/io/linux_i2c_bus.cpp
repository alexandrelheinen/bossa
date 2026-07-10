/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/io/linux_i2c_bus.hpp"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace bossa::io {

LinuxI2cBus::LinuxI2cBus() = default;

LinuxI2cBus::~LinuxI2cBus() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool LinuxI2cBus::open(std::string_view device_path) {
    if (device_path.empty()) {
        return false;
    }

    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }

    const std::string path(device_path);
    fd_ = ::open(path.c_str(), O_RDWR);
    if (fd_ < 0) {
        return false;
    }

    device_path_ = path;
    return true;
}

bool LinuxI2cBus::write_read(std::uint8_t address,
                             std::span<const std::uint8_t> write_data,
                             std::span<std::uint8_t> read_data) {
    if (fd_ < 0 || write_data.empty() || read_data.empty()) {
        return false;
    }

    i2c_msg messages[2]{};
    messages[0].addr = address;
    messages[0].flags = 0;
    messages[0].len = static_cast<__u16>(write_data.size());
    messages[0].buf = const_cast<std::uint8_t *>(write_data.data());

    messages[1].addr = address;
    messages[1].flags = I2C_M_RD;
    messages[1].len = static_cast<__u16>(read_data.size());
    messages[1].buf = read_data.data();

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs = messages;
    ioctl_data.nmsgs = 2;

    if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) {
        return false;
    }
    return true;
}

bool LinuxI2cBus::write(std::uint8_t address,
                        std::span<const std::uint8_t> write_data) {
    if (fd_ < 0 || write_data.empty()) {
        return false;
    }

    i2c_msg message{};
    message.addr = address;
    message.flags = 0;
    message.len = static_cast<__u16>(write_data.size());
    message.buf = const_cast<std::uint8_t *>(write_data.data());

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs = &message;
    ioctl_data.nmsgs = 1;

    if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) {
        return false;
    }
    return true;
}

} // namespace bossa::io
