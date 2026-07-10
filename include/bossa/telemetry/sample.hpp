/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <string_view>

namespace bossa::telemetry {

enum class SampleQuality { kGood, kUncertain, kBad };

/**
 * @brief A single sensor reading with metadata.
 *
 * channel_id and unit are non-owning views; the producer must keep backing
 * storage alive for the lifetime of the Sample (typically driver member strings
 * set during configure()).
 */
struct Sample {
    std::string_view channel_id;
    double value{0.0};
    std::chrono::system_clock::time_point timestamp{};
    std::string_view unit;
    SampleQuality quality{SampleQuality::kGood};
};

} // namespace bossa::telemetry
