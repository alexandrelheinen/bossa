/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstring>

#include "bossa/telemetry/channel.hpp"
#include "bossa/telemetry/sample.hpp"

namespace bossa::telemetry {

/**
 * @brief Owning sample stored in RingBuffer / LocalStore.
 *
 * Fixed char arrays avoid heap allocation after construction.
 */
struct StoredSample {
    static constexpr std::size_t kMaxChannelIdLength = 63;
    static constexpr std::size_t kMaxUnitLength = 31;

    char channel_id[kMaxChannelIdLength + 1]{};
    char unit[kMaxUnitLength + 1]{};
    double value{0.0};
    std::chrono::system_clock::time_point timestamp{};
    SampleQuality quality{SampleQuality::kGood};
    Priority priority{Priority::kNormal};

    /**
     * @brief Copy a non-owning Sample into a StoredSample.
     * @param sample Source sample (string_views must be valid).
     * @param sample_priority Priority used for overflow policy.
     * @return Populated StoredSample.
     */
    static StoredSample from_sample(const Sample &sample,
                                    Priority sample_priority) {
        StoredSample stored;
        copy_truncated(stored.channel_id, sizeof(stored.channel_id),
                       sample.channel_id);
        copy_truncated(stored.unit, sizeof(stored.unit), sample.unit);
        stored.value = sample.value;
        stored.timestamp = sample.timestamp;
        stored.quality = sample.quality;
        stored.priority = sample_priority;
        return stored;
    }

  private:
    static void copy_truncated(char *destination, std::size_t capacity,
                               std::string_view source) {
        if (capacity == 0) {
            return;
        }
        const std::size_t copy_length =
            source.size() < (capacity - 1) ? source.size() : (capacity - 1);
        if (copy_length > 0) {
            std::memcpy(destination, source.data(), copy_length);
        }
        destination[copy_length] = '\0';
    }
};

} // namespace bossa::telemetry
