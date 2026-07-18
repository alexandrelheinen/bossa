/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>

#include "bossa/telemetry/channel.hpp"

namespace bossa::sync {

/** @brief Per-channel sync parameters used by UploadPolicy. */
struct ChannelSyncState {
    std::string channel_id;
    telemetry::SyncMode mode{telemetry::SyncMode::kBatch};
    telemetry::Priority priority{telemetry::Priority::kNormal};
    std::chrono::seconds remote_interval{60};
    double on_change_threshold{0.0};
    bool destination_remote{true};
};

/**
 * @brief Decides when buffered samples should be uploaded.
 */
class UploadPolicy {
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    /**
     * @brief Register or replace channel sync state.
     * @param state Channel parameters.
     */
    void configure_channel(ChannelSyncState state);

    /**
     * @brief Clear all channel state (used on config reload).
     */
    void clear();

    /**
     * @brief Record that a sample arrived for a channel.
     * @param channel_id Channel id.
     * @param value Latest sample value.
     * @param now Current time.
     */
    void on_sample(const std::string &channel_id, double value, TimePoint now);

    /**
     * @brief Whether an upload flush should run now.
     * @param now Current time.
     * @param buffered_count Samples waiting in the ring buffer.
     * @param max_batch_size Configured max batch size.
     * @return true when the uploader should drain the buffer.
     */
    bool should_flush(TimePoint now, std::size_t buffered_count,
                      std::size_t max_batch_size) const;

    /**
     * @brief Mark that a flush completed successfully.
     * @param now Current time.
     */
    void mark_flushed(TimePoint now);

  private:
    struct RuntimeState {
        ChannelSyncState config;
        TimePoint last_flush{};
        double last_uploaded_value{0.0};
        bool has_uploaded_value{false};
        bool pending_realtime{false};
        bool pending_on_change{false};
    };

    std::unordered_map<std::string, RuntimeState> channels_;
    bool any_pending_realtime_{false};
    bool any_pending_on_change_{false};
};

} // namespace bossa::sync
