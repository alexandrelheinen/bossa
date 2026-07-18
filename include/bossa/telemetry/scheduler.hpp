/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace bossa::telemetry {

/**
 * @brief Per-channel schedule entry (id + sample rate).
 */
struct ScheduleEntry {
    std::string channel_id;
    double sample_rate_hz{1.0};
};

/**
 * @brief Deadline-based channel poll scheduler.
 *
 * Uses an injectable clock for deterministic tests. Missed deadlines skip
 * catch-up (advance to the next future slot).
 */
class Scheduler {
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using ClockFn = std::function<TimePoint()>;
    using PollFn = std::function<void(const std::string &channel_id)>;

    /**
     * @brief Construct a scheduler.
     * @param now Clock function (defaults to steady_clock::now).
     */
    explicit Scheduler(ClockFn now = []() { return Clock::now(); });

    /**
     * @brief Replace the channel schedule (used at start and on SIGHUP).
     * @param entries Channel ids and rates.
     */
    void configure(std::vector<ScheduleEntry> entries);

    /**
     * @brief Poll every channel whose deadline has passed.
     * @param poll Callback invoked once per due channel.
     * @return Number of channels polled.
     */
    std::size_t dispatch_due(const PollFn &poll);

    /**
     * @brief Next deadline among configured channels.
     * @return Time point, or nullopt when no channels are configured.
     */
    std::optional<TimePoint> next_deadline() const;

    /** @brief Number of configured channels. */
    std::size_t channel_count() const;

  private:
    struct ChannelState {
        ScheduleEntry entry;
        TimePoint next_deadline{};
        Clock::duration period{Clock::duration::zero()};
    };

    static Clock::duration period_from_hz(double sample_rate_hz);

    ClockFn now_;
    std::vector<ChannelState> channels_;
};

} // namespace bossa::telemetry
