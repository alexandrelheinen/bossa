/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "bossa/io/i2c_bus.hpp"

namespace bossa::io {

/**
 * @brief Scriptable I2C mock for driver unit tests.
 */
class MockI2cBus : public I2cBus {
  public:
    using TransactionHandler = std::function<bool(
        std::uint8_t address, std::span<const std::uint8_t> write_data,
        std::span<std::uint8_t> read_data)>;

    bool open(std::string_view device_path) override;
    bool write_read(std::uint8_t address,
                    std::span<const std::uint8_t> write_data,
                    std::span<std::uint8_t> read_data) override;
    bool write(std::uint8_t address,
               std::span<const std::uint8_t> write_data) override;

    /**
     * @brief Register a handler keyed by the first write byte (register
     * address).
     */
    void set_register_handler(std::uint8_t register_address,
                              TransactionHandler handler);

    void set_default_handler(TransactionHandler handler);

    const std::string &device_path() const { return device_path_; }

  private:
    std::string device_path_;
    std::unordered_map<std::uint8_t, TransactionHandler> register_handlers_;
    TransactionHandler default_handler_;
};

} // namespace bossa::io
