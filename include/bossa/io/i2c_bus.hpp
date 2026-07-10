/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace bossa::io {

/**
 * @brief Virtual I2C bus for sensor drivers and unit tests.
 */
class I2cBus {
  public:
    virtual ~I2cBus() = default;

    /**
     * @brief Open a Linux I2C adapter device path.
     * @return true when the bus is ready for transactions.
     */
    virtual bool open(std::string_view device_path) = 0;

    /**
     * @brief Write bytes to a device, then read bytes (repeated-start).
     * @param address 7-bit I2C address.
     */
    virtual bool write_read(std::uint8_t address,
                            std::span<const std::uint8_t> write_data,
                            std::span<std::uint8_t> read_data) = 0;

    /**
     * @brief Write bytes to a device without a following read.
     */
    virtual bool write(std::uint8_t address,
                       std::span<const std::uint8_t> write_data) = 0;
};

} // namespace bossa::io
