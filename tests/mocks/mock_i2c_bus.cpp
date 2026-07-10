/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <array>
#include <cstdint>

#include "mock_i2c_bus.hpp"

namespace bossa::io {

bool MockI2cBus::open(std::string_view device_path) {
    if (device_path.empty()) {
        return false;
    }
    device_path_ = std::string(device_path);
    return true;
}

bool MockI2cBus::write_read(std::uint8_t address,
                            std::span<const std::uint8_t> write_data,
                            std::span<std::uint8_t> read_data) {
    if (write_data.empty()) {
        return false;
    }

    const std::uint8_t register_address = write_data[0];
    const auto iterator = register_handlers_.find(register_address);
    if (iterator != register_handlers_.end()) {
        return iterator->second(address, write_data, read_data);
    }

    if (default_handler_) {
        return default_handler_(address, write_data, read_data);
    }
    return false;
}

bool MockI2cBus::write(std::uint8_t address,
                       std::span<const std::uint8_t> write_data) {
    std::array<std::uint8_t, 1> dummy_read{};
    return write_read(address, write_data, dummy_read);
}

void MockI2cBus::set_register_handler(std::uint8_t register_address,
                                      TransactionHandler handler) {
    register_handlers_[register_address] = std::move(handler);
}

void MockI2cBus::set_default_handler(TransactionHandler handler) {
    default_handler_ = std::move(handler);
}

} // namespace bossa::io
