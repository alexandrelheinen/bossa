/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace bossa::io {

enum class GpioDirection { kInput, kOutput };
enum class GpioValue { kLow, kHigh };

/**
 * @brief Virtual GPIO line controller for testable hardware access.
 */
class GpioController {
  public:
    virtual ~GpioController() = default;

    /**
     * @brief Request exclusive access to a GPIO line.
     * @param chip_name GPIO chip device name (e.g. "gpiochip4").
     * @param offset Line offset on the chip.
     * @param direction Requested line direction.
     * @return true on success.
     */
    virtual bool request_line(std::string_view chip_name, std::uint32_t offset,
                              GpioDirection direction) = 0;

    /**
     * @brief Read the current value of a previously requested line.
     */
    virtual bool read_line(std::uint32_t offset, GpioValue *value) = 0;

    /**
     * @brief Drive an output line high or low.
     */
    virtual bool write_line(std::uint32_t offset, GpioValue value) = 0;
};

} // namespace bossa::io
