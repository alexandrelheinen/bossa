/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "fake_tsl2561_driver.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

#include "bossa/drivers/registry.hpp"

namespace bossa::drivers {

std::string_view FakeTsl2561Driver::name() const { return "fake-tsl2561"; }

void FakeTsl2561Driver::configure(const nlohmann::json &parameters) {
    if (parameters.contains("lux_id"))
        lux_id_ = parameters["lux_id"].get<std::string>();
    sample_index_ = 0;
    configured_ = true;
}

ReadResult FakeTsl2561Driver::read() {
    ReadResult result;
    if (!configured_)
        return result;
    const auto timestamp = std::chrono::system_clock::now();

    result.samples[0].channel_id = lux_id_;
    result.samples[0].unit = "lux";
    result.samples[0].value = 500.0 + 200.0 * std::sin(sample_index_ * 0.05);
    result.samples[0].timestamp = timestamp;
    result.samples[0].quality = telemetry::SampleQuality::kGood;

    result.sample_count = 1;
    ++sample_index_;
    return result;
}

void FakeTsl2561Driver::write(const nlohmann::json &) {}

} // namespace bossa::drivers

BOSSA_REGISTER_DRIVER(bossa::drivers::FakeTsl2561Driver, "fake-tsl2561")
void bossa_force_link_fake_tsl2561_driver() {}
