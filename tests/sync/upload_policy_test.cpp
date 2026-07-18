/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include "bossa/sync/upload_policy.hpp"

TEST(UploadPolicyTest, BatchFlushesAfterInterval) {
    bossa::sync::UploadPolicy policy;
    bossa::sync::ChannelSyncState state;
    state.channel_id = "temp";
    state.mode = bossa::telemetry::SyncMode::kBatch;
    state.remote_interval = std::chrono::seconds(60);
    state.destination_remote = true;
    policy.configure_channel(state);

    using clock = bossa::sync::UploadPolicy::Clock;
    const auto start = clock::now();
    policy.mark_flushed(start);

    EXPECT_FALSE(policy.should_flush(start + std::chrono::seconds(30), 3, 500));
    EXPECT_TRUE(policy.should_flush(start + std::chrono::seconds(60), 3, 500));
}

TEST(UploadPolicyTest, RealtimeFlushesImmediately) {
    bossa::sync::UploadPolicy policy;
    bossa::sync::ChannelSyncState state;
    state.channel_id = "alarm";
    state.mode = bossa::telemetry::SyncMode::kRealtime;
    state.destination_remote = true;
    policy.configure_channel(state);

    using clock = bossa::sync::UploadPolicy::Clock;
    const auto now = clock::now();
    policy.on_sample("alarm", 1.0, now);
    EXPECT_TRUE(policy.should_flush(now, 1, 500));
}
