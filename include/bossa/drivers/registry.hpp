/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "bossa/drivers/driver.hpp"

namespace bossa::drivers {

/**
 * @brief Static factory registry for built-in drivers.
 */
class Registry {
  public:
    using Factory = std::function<std::unique_ptr<Driver>()>;

    static bool register_driver(std::string_view name, Factory factory);
    static std::unique_ptr<Driver> create(std::string_view name);
    static bool contains(std::string_view name);
};

#define BOSSA_REGISTER_DRIVER(DriverClass, driver_name)                        \
    namespace {                                                                \
    const bool BOSSA_CONCAT(_bossa_registered_, __LINE__) =                    \
        ::bossa::drivers::Registry::register_driver(driver_name, []() {        \
            return std::make_unique<DriverClass>();                            \
        });                                                                    \
    }

#define BOSSA_CONCAT_INNER(a, b) a##b
#define BOSSA_CONCAT(a, b) BOSSA_CONCAT_INNER(a, b)

} // namespace bossa::drivers
