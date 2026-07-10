/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "mock_gpio.hpp"

namespace bossa::io {

bool MockGpio::request_line(std::string_view chip_name, std::uint32_t offset,
                            GpioDirection direction) {
    chip_name_ = std::string(chip_name);
    auto &line = lines_[offset];
    line.direction = direction;
    line.requested = true;
    return true;
}

bool MockGpio::read_line(std::uint32_t offset, GpioValue *value) {
    if (value == nullptr) {
        return false;
    }

    const auto iterator = lines_.find(offset);
    if (iterator == lines_.end() || !iterator->second.requested) {
        return false;
    }

    *value = iterator->second.value;
    return true;
}

bool MockGpio::write_line(std::uint32_t offset, GpioValue value) {
    auto iterator = lines_.find(offset);
    if (iterator == lines_.end() || !iterator->second.requested ||
        iterator->second.direction != GpioDirection::kOutput) {
        return false;
    }

    iterator->second.value = value;
    return true;
}

void MockGpio::set_line_value(std::uint32_t offset, GpioValue value) {
    lines_[offset].value = value;
}

} // namespace bossa::io
