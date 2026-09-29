/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "fake_bme280_driver.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

#include "bossa/drivers/registry.hpp"

namespace bossa::drivers {

std::string_view FakeBme280Driver::name() const { return "fake-bme280"; }

void FakeBme280Driver::configure(const nlohmann::json &parameters) {
    if (parameters.contains("temperature_channel_id")) {
        temp_channel_id_ =
            parameters["temperature_channel_id"].get<std::string>();
    }
    if (parameters.contains("humidity_channel_id")) {
        hum_channel_id_ = parameters["humidity_channel_id"].get<std::string>();
    }
    if (parameters.contains("pressure_channel_id")) {
        press_channel_id_ =
            parameters["pressure_channel_id"].get<std::string>();
    }
    sample_index_ = 0;
    configured_ = true;
}

ReadResult FakeBme280Driver::read() {
    ReadResult result;
    if (!configured_) {
        return result;
    }
    const auto timestamp = std::chrono::system_clock::now();
    double temp_val = 22.0 + 2.0 * std::sin(sample_index_ * 0.1);
    double hum_val = 45.0 + 5.0 * std::cos(sample_index_ * 0.1);
    double press_val = 1013.25 + 1.0 * std::sin(sample_index_ * 0.05);

    result.samples[0].channel_id = temp_channel_id_;
    result.samples[0].unit = "celsius";
    result.samples[0].value = temp_val;
    result.samples[0].timestamp = timestamp;
    result.samples[0].quality = telemetry::SampleQuality::kGood;

    result.samples[1].channel_id = hum_channel_id_;
    result.samples[1].unit = "percent";
    result.samples[1].value = hum_val;
    result.samples[1].timestamp = timestamp;
    result.samples[1].quality = telemetry::SampleQuality::kGood;

    result.samples[2].channel_id = press_channel_id_;
    result.samples[2].unit = "hPa";
    result.samples[2].value = press_val;
    result.samples[2].timestamp = timestamp;
    result.samples[2].quality = telemetry::SampleQuality::kGood;

    result.sample_count = 3;
    ++sample_index_;
    return result;
}

void FakeBme280Driver::write(const nlohmann::json & /*command*/) {}

} // namespace bossa::drivers

BOSSA_REGISTER_DRIVER(bossa::drivers::FakeBme280Driver, "fake-bme280")

void bossa_force_link_fake_bme280_driver() {}
