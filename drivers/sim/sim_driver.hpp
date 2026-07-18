/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include "bossa/drivers/driver.hpp"

namespace bossa::drivers {

/**
 * @brief Software sensor that emits deterministic samples (no hardware).
 *
 * YAML parameters:
 * - channel_id (string, required)
 * - unit (string, default "count")
 * - base_value (number, default 0)
 * - step (number, default 1)
 */
class SimDriver : public Driver {
  public:
    std::string_view name() const override;
    void configure(const nlohmann::json &parameters) override;
    ReadResult read() override;
    void write(const nlohmann::json &command) override;

  private:
    bool configured_{false};
    std::string channel_id_;
    std::string unit_{"count"};
    double base_value_{0.0};
    double step_{1.0};
    std::uint64_t sample_index_{0};
};

} // namespace bossa::drivers
