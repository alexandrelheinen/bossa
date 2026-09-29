/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include "bossa/drivers/driver.hpp"

namespace bossa::drivers {

class FakeMpu6050Driver : public Driver {
  public:
    std::string_view name() const override;
    void configure(const nlohmann::json &parameters) override;
    ReadResult read() override;
    void write(const nlohmann::json &command) override;

  private:
    bool configured_{false};
    std::string accel_x_id_{"accel_x"};
    std::string accel_y_id_{"accel_y"};
    std::string accel_z_id_{"accel_z"};
    std::uint64_t sample_index_{0};
};

} // namespace bossa::drivers
