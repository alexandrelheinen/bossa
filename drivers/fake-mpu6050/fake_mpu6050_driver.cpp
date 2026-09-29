/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "fake_mpu6050_driver.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

#include "bossa/drivers/registry.hpp"

namespace bossa::drivers {

std::string_view FakeMpu6050Driver::name() const { return "fake-mpu6050"; }

void FakeMpu6050Driver::configure(const nlohmann::json &parameters) {
    if (parameters.contains("accel_x_id"))
        accel_x_id_ = parameters["accel_x_id"].get<std::string>();
    if (parameters.contains("accel_y_id"))
        accel_y_id_ = parameters["accel_y_id"].get<std::string>();
    if (parameters.contains("accel_z_id"))
        accel_z_id_ = parameters["accel_z_id"].get<std::string>();
    sample_index_ = 0;
    configured_ = true;
}

ReadResult FakeMpu6050Driver::read() {
    ReadResult result;
    if (!configured_)
        return result;
    const auto timestamp = std::chrono::system_clock::now();

    result.samples[0].channel_id = accel_x_id_;
    result.samples[0].unit = "g";
    result.samples[0].value = 0.0 + 0.1 * std::sin(sample_index_ * 0.2);
    result.samples[0].timestamp = timestamp;
    result.samples[0].quality = telemetry::SampleQuality::kGood;

    result.samples[1].channel_id = accel_y_id_;
    result.samples[1].unit = "g";
    result.samples[1].value = 0.0 + 0.1 * std::cos(sample_index_ * 0.2);
    result.samples[1].timestamp = timestamp;
    result.samples[1].quality = telemetry::SampleQuality::kGood;

    result.samples[2].channel_id = accel_z_id_;
    result.samples[2].unit = "g";
    result.samples[2].value = 1.0 + 0.05 * std::sin(sample_index_ * 0.1);
    result.samples[2].timestamp = timestamp;
    result.samples[2].quality = telemetry::SampleQuality::kGood;

    result.sample_count = 3;
    ++sample_index_;
    return result;
}

void FakeMpu6050Driver::write(const nlohmann::json &) {}

} // namespace bossa::drivers

BOSSA_REGISTER_DRIVER(bossa::drivers::FakeMpu6050Driver, "fake-mpu6050")
void bossa_force_link_fake_mpu6050_driver() {}
