/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "sim_driver.hpp"

#include <chrono>
#include <cstdint>
#include <stdexcept>

#include "bossa/drivers/registry.hpp"

namespace bossa::drivers {

std::string_view SimDriver::name() const { return "sim"; }

void SimDriver::configure(const nlohmann::json &parameters) {
    if (!parameters.contains("channel_id") ||
        !parameters["channel_id"].is_string() ||
        parameters["channel_id"].get<std::string>().empty()) {
        throw std::runtime_error("sim driver requires non-empty channel_id");
    }
    channel_id_ = parameters["channel_id"].get<std::string>();
    if (parameters.contains("unit") && parameters["unit"].is_string()) {
        unit_ = parameters["unit"].get<std::string>();
    }
    if (parameters.contains("base_value") && parameters["base_value"].is_number()) {
        base_value_ = parameters["base_value"].get<double>();
    }
    if (parameters.contains("step") && parameters["step"].is_number()) {
        step_ = parameters["step"].get<double>();
    }
    sample_index_ = 0;
    configured_ = true;
}

ReadResult SimDriver::read() {
    ReadResult result;
    if (!configured_) {
        return result;
    }
    telemetry::Sample sample;
    sample.channel_id = channel_id_;
    sample.unit = unit_;
    sample.value = base_value_ + (static_cast<double>(sample_index_) * step_);
    sample.timestamp = std::chrono::system_clock::now();
    sample.quality = telemetry::SampleQuality::kGood;
    result.samples[0] = sample;
    result.sample_count = 1;
    ++sample_index_;
    return result;
}

void SimDriver::write(const nlohmann::json & /*command*/) {
    // Actuator path unused for the simulator.
}

} // namespace bossa::drivers

BOSSA_REGISTER_DRIVER(bossa::drivers::SimDriver, "sim")

// Ensure static registration is not dropped from libbossa_drivers.a.
void bossa_force_link_sim_driver() {}
