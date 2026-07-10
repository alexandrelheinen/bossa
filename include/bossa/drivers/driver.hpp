/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "bossa/telemetry/sample.hpp"

namespace bossa::drivers {

/**
 * @brief Fixed-capacity result from Driver::read() — no heap growth in hot
 * path.
 */
struct ReadResult {
    static constexpr std::size_t kMaxSamples = 8;

    std::array<telemetry::Sample, kMaxSamples> samples{};
    std::size_t sample_count{0};
};

/**
 * @brief Hardware driver interface.
 *
 * Drivers must not allocate memory in read() or write() after configure()
 * completes.
 */
class Driver {
  public:
    virtual ~Driver() = default;

    /** @brief Unique driver type name, e.g. "bme280". */
    virtual std::string_view name() const = 0;

    /**
     * @brief One-time setup from JSON parameters block.
     * @throws std::runtime_error on invalid configuration (not in hot path).
     */
    virtual void configure(const nlohmann::json &parameters) = 0;

    /**
     * @brief Read current samples from hardware.
     * @return One Sample per logical channel this driver exposes.
     */
    virtual ReadResult read() = 0;

    /**
     * @brief Execute an actuator command.
     * @param command JSON payload, driver-specific schema.
     */
    virtual void write(const nlohmann::json &command) = 0;
};

} // namespace bossa::drivers
