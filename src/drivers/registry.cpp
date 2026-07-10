/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/drivers/registry.hpp"

#include <mutex>
#include <unordered_map>

namespace bossa::drivers {

namespace {

std::unordered_map<std::string, Registry::Factory> &driver_factories() {
    static std::unordered_map<std::string, Registry::Factory> factories;
    return factories;
}

std::mutex &registry_mutex() {
    static std::mutex mutex;
    return mutex;
}

} // namespace

bool Registry::register_driver(std::string_view name, Factory factory) {
    if (name.empty() || !factory) {
        return false;
    }

    const std::lock_guard<std::mutex> lock(registry_mutex());
    driver_factories()[std::string(name)] = std::move(factory);
    return true;
}

std::unique_ptr<Driver> Registry::create(std::string_view name) {
    const std::lock_guard<std::mutex> lock(registry_mutex());
    const auto iterator = driver_factories().find(std::string(name));
    if (iterator == driver_factories().end()) {
        return nullptr;
    }
    return iterator->second();
}

bool Registry::contains(std::string_view name) {
    const std::lock_guard<std::mutex> lock(registry_mutex());
    return driver_factories().contains(std::string(name));
}

} // namespace bossa::drivers
