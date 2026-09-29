/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include "bossa/drivers/driver.hpp"

namespace bossa::drivers {

/**
 * @brief Fake BME280 driver that simulates 3 channels (temperature, humidity,
 * pressure).
 */
class FakeBme280Driver : public Driver {
  public:
    std::string_view name() const override;
    void configure(const nlohmann::json &parameters) override;
    ReadResult read() override;
    void write(const nlohmann::json &command) override;

  private:
    bool configured_{false};
    std::string temp_channel_id_{"temperature"};
    std::string hum_channel_id_{"humidity"};
    std::string press_channel_id_{"pressure"};
    std::uint64_t sample_index_{0};
};

} // namespace bossa::drivers
