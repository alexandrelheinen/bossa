/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/sync/upload_policy.hpp"

#include <cmath>

namespace bossa::sync {

void UploadPolicy::configure_channel(ChannelSyncState state) {
    RuntimeState runtime;
    runtime.config = std::move(state);
    runtime.last_flush = Clock::now();
    channels_[runtime.config.channel_id] = std::move(runtime);
}

void UploadPolicy::clear() {
    channels_.clear();
    any_pending_realtime_ = false;
    any_pending_on_change_ = false;
}

void UploadPolicy::on_sample(const std::string &channel_id, double value,
                             TimePoint now) {
    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return;
    }
    auto &state = it->second;
    if (!state.config.destination_remote) {
        return;
    }

    switch (state.config.mode) {
    case telemetry::SyncMode::kRealtime:
        state.pending_realtime = true;
        any_pending_realtime_ = true;
        break;
    case telemetry::SyncMode::kOnChange:
        if (!state.has_uploaded_value ||
            std::abs(value - state.last_uploaded_value) >
                state.config.on_change_threshold) {
            state.pending_on_change = true;
            any_pending_on_change_ = true;
            state.last_uploaded_value = value;
            state.has_uploaded_value = true;
        }
        break;
    case telemetry::SyncMode::kBatch:
        (void)now;
        break;
    }
}

bool UploadPolicy::should_flush(TimePoint now, std::size_t buffered_count,
                                std::size_t max_batch_size) const {
    if (buffered_count == 0) {
        return false;
    }
    if (max_batch_size > 0 && buffered_count >= max_batch_size) {
        return true;
    }
    if (any_pending_realtime_ || any_pending_on_change_) {
        return true;
    }

    for (const auto &[_, state] : channels_) {
        if (!state.config.destination_remote) {
            continue;
        }
        if (state.config.mode != telemetry::SyncMode::kBatch) {
            continue;
        }
        if (now - state.last_flush >= state.config.remote_interval) {
            return true;
        }
    }
    return false;
}

void UploadPolicy::mark_flushed(TimePoint now) {
    any_pending_realtime_ = false;
    any_pending_on_change_ = false;
    for (auto &[_, state] : channels_) {
        state.last_flush = now;
        state.pending_realtime = false;
        state.pending_on_change = false;
    }
}

} // namespace bossa::sync
