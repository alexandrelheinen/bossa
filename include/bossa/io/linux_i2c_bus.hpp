/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include "bossa/io/i2c_bus.hpp"

namespace bossa::io {

/**
 * @brief Linux i2c-dev backend using ioctl I2C_RDWR.
 */
class LinuxI2cBus : public I2cBus {
  public:
    LinuxI2cBus();
    ~LinuxI2cBus() override;

    LinuxI2cBus(const LinuxI2cBus &) = delete;
    LinuxI2cBus &operator=(const LinuxI2cBus &) = delete;

    bool open(std::string_view device_path) override;
    bool write_read(std::uint8_t address,
                    std::span<const std::uint8_t> write_data,
                    std::span<std::uint8_t> read_data) override;
    bool write(std::uint8_t address,
               std::span<const std::uint8_t> write_data) override;

  private:
    int fd_{-1};
    std::string device_path_;
};

} // namespace bossa::io
