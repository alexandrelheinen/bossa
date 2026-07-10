/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/io/libgpiod_gpio.hpp"

#if defined(HAVE_LIBGPIOD)

#include <gpiod.h>

namespace bossa::io {

struct LibgpiodGpio::LineHandle {
    gpiod_chip *chip{nullptr};
    gpiod_line *line{nullptr};
};

LibgpiodGpio::LibgpiodGpio() = default;

LibgpiodGpio::~LibgpiodGpio() {
    for (auto &[offset, handle] : lines_) {
        (void)offset;
        if (handle != nullptr) {
            if (handle->line != nullptr) {
                gpiod_line_release(handle->line);
            }
            if (handle->chip != nullptr) {
                gpiod_chip_close(handle->chip);
            }
        }
    }
    lines_.clear();
}

bool LibgpiodGpio::request_line(std::string_view chip_name,
                                std::uint32_t offset, GpioDirection direction) {
    if (chip_name.empty()) {
        return false;
    }

    if (lines_.contains(offset)) {
        return true;
    }

    gpiod_chip *chip = gpiod_chip_open_by_name(std::string(chip_name).c_str());
    if (chip == nullptr) {
        return false;
    }

    gpiod_line *line = gpiod_chip_get_line(chip, offset);
    if (line == nullptr) {
        gpiod_chip_close(chip);
        return false;
    }

    int request_status = -1;
    if (direction == GpioDirection::kInput) {
        request_status = gpiod_line_request_input(line, "bossa-daemon");
    } else {
        request_status = gpiod_line_request_output(line, "bossa-daemon",
                                                   GPIOD_LINE_ACTIVE_STATE_LOW);
    }

    if (request_status != 0) {
        gpiod_chip_close(chip);
        return false;
    }

    auto handle = std::make_unique<LineHandle>();
    handle->chip = chip;
    handle->line = line;
    chip_name_ = std::string(chip_name);
    lines_.emplace(offset, std::move(handle));
    return true;
}

bool LibgpiodGpio::read_line(std::uint32_t offset, GpioValue *value) {
    if (value == nullptr || !lines_.contains(offset)) {
        return false;
    }

    const int raw_value = gpiod_line_get_value(lines_[offset]->line);
    if (raw_value < 0) {
        return false;
    }

    *value = raw_value == 0 ? GpioValue::kLow : GpioValue::kHigh;
    return true;
}

bool LibgpiodGpio::write_line(std::uint32_t offset, GpioValue value) {
    if (!lines_.contains(offset)) {
        return false;
    }

    const int raw_value = value == GpioValue::kLow ? 0 : 1;
    return gpiod_line_set_value(lines_[offset]->line, raw_value) == 0;
}

} // namespace bossa::io

#else

namespace bossa::io {

struct LibgpiodGpio::LineHandle {};

LibgpiodGpio::LibgpiodGpio() = default;
LibgpiodGpio::~LibgpiodGpio() = default;

bool LibgpiodGpio::request_line(std::string_view, std::uint32_t,
                                GpioDirection) {
    return false;
}

bool LibgpiodGpio::read_line(std::uint32_t, GpioValue *) { return false; }

bool LibgpiodGpio::write_line(std::uint32_t, GpioValue) { return false; }

} // namespace bossa::io

#endif
