/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "bossa/io/gpio_controller.hpp"

namespace bossa::io {

/**
 * @brief libgpiod character-device GPIO backend (v1 API).
 */
class LibgpiodGpio : public GpioController {
  public:
    LibgpiodGpio();
    ~LibgpiodGpio() override;

    LibgpiodGpio(const LibgpiodGpio &) = delete;
    LibgpiodGpio &operator=(const LibgpiodGpio &) = delete;

    bool request_line(std::string_view chip_name, std::uint32_t offset,
                      GpioDirection direction) override;
    bool read_line(std::uint32_t offset, GpioValue *value) override;
    bool write_line(std::uint32_t offset, GpioValue value) override;

  private:
    struct LineHandle;

    std::unordered_map<std::uint32_t, std::unique_ptr<LineHandle>> lines_;
    std::string chip_name_;
};

} // namespace bossa::io
