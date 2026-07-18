/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/telemetry/scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <syslog.h>

namespace bossa::telemetry {

Scheduler::Scheduler(ClockFn now) : now_(std::move(now)) {}

Scheduler::Clock::duration Scheduler::period_from_hz(double sample_rate_hz) {
    if (!(sample_rate_hz > 0.0) || !std::isfinite(sample_rate_hz)) {
        return std::chrono::seconds(1);
    }
    const double period_seconds = 1.0 / sample_rate_hz;
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(period_seconds));
}

void Scheduler::configure(std::vector<ScheduleEntry> entries) {
    const TimePoint now = now_();
    channels_.clear();
    channels_.reserve(entries.size());
    for (auto &entry : entries) {
        ChannelState state;
        state.entry = std::move(entry);
        state.period = period_from_hz(state.entry.sample_rate_hz);
        state.next_deadline = now; // due immediately on configure / reload
        channels_.push_back(std::move(state));
    }
}

std::size_t Scheduler::dispatch_due(const PollFn &poll) {
    if (!poll) {
        return 0;
    }

    const TimePoint now = now_();
    std::size_t polled = 0;

    for (auto &channel : channels_) {
        if (channel.next_deadline > now) {
            continue;
        }

        poll(channel.entry.channel_id);
        ++polled;

        // Skip catch-up storms: advance to the next future slot.
        auto next = channel.next_deadline + channel.period;
        while (next <= now) {
            next += channel.period;
        }
        if (next <= channel.next_deadline) {
            // Degenerate period; force a small advance.
            next = now + channel.period;
            syslog(LOG_WARNING,
                   "scheduler: non-advancing period for channel %s",
                   channel.entry.channel_id.c_str());
        }
        channel.next_deadline = next;
    }

    return polled;
}

std::optional<Scheduler::TimePoint> Scheduler::next_deadline() const {
    if (channels_.empty()) {
        return std::nullopt;
    }
    auto it = std::min_element(
        channels_.begin(), channels_.end(),
        [](const ChannelState &left, const ChannelState &right) {
            return left.next_deadline < right.next_deadline;
        });
    return it->next_deadline;
}

std::size_t Scheduler::channel_count() const { return channels_.size(); }

} // namespace bossa::telemetry
