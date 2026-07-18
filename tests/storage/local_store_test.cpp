/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <filesystem>

#include <gtest/gtest.h>

#include "bossa/storage/local_store.hpp"

namespace {

bossa::telemetry::StoredSample make_sample(const char *channel, double value) {
    bossa::telemetry::Sample sample;
    sample.channel_id = channel;
    sample.unit = "celsius";
    sample.value = value;
    sample.timestamp = std::chrono::system_clock::now();
    return bossa::telemetry::StoredSample::from_sample(
        sample, bossa::telemetry::Priority::kNormal);
}

} // namespace

TEST(LocalStoreTest, EnqueuePeekAcknowledge) {
    const auto path =
        std::filesystem::temp_directory_path() / "bossa_local_store_test.db";
    std::filesystem::remove(path);

    bossa::storage::LocalStore store;
    ASSERT_TRUE(store.open(path)) << store.last_error();
    ASSERT_TRUE(store.enqueue(make_sample("ambient_temperature", 22.5)));
    ASSERT_TRUE(store.enqueue(make_sample("ambient_humidity", 55.0)));
    EXPECT_EQ(store.pending_count(), 2u);

    const auto batch = store.peek_batch(10);
    ASSERT_EQ(batch.size(), 2u);
    EXPECT_STREQ(batch[0].sample.channel_id, "ambient_temperature");
    EXPECT_DOUBLE_EQ(batch[0].sample.value, 22.5);

    ASSERT_TRUE(store.acknowledge(batch[0].id));
    EXPECT_EQ(store.pending_count(), 1u);

    store.close();
    std::filesystem::remove(path);
}
