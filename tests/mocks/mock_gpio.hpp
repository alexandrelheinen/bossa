/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "bossa/io/gpio_controller.hpp"

namespace bossa::io {

/**
 * @brief In-memory GPIO mock for unit tests.
 */
class MockGpio : public GpioController {
  public:
    bool request_line(std::string_view chip_name, std::uint32_t offset,
                      GpioDirection direction) override;
    bool read_line(std::uint32_t offset, GpioValue *value) override;
    bool write_line(std::uint32_t offset, GpioValue value) override;

    void set_line_value(std::uint32_t offset, GpioValue value);

  private:
    struct LineState {
        GpioDirection direction{GpioDirection::kInput};
        GpioValue value{GpioValue::kLow};
        bool requested{false};
    };

    std::string chip_name_;
    std::unordered_map<std::uint32_t, LineState> lines_;
};

} // namespace bossa::io
