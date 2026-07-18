/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include "bossa/telemetry/ring_buffer.hpp"

namespace {

bossa::telemetry::StoredSample make_sample(const char *channel,
                                           bossa::telemetry::Priority priority,
                                           double value) {
    bossa::telemetry::Sample sample;
    sample.channel_id = channel;
    sample.unit = "celsius";
    sample.value = value;
    sample.timestamp = std::chrono::system_clock::now();
    sample.quality = bossa::telemetry::SampleQuality::kGood;
    return bossa::telemetry::StoredSample::from_sample(sample, priority);
}

} // namespace

TEST(RingBufferTest, EnqueueDequeueFifo) {
    bossa::telemetry::RingBuffer buffer(3);
    ASSERT_TRUE(buffer.enqueue(
        make_sample("a", bossa::telemetry::Priority::kNormal, 1.0)));
    ASSERT_TRUE(buffer.enqueue(
        make_sample("b", bossa::telemetry::Priority::kNormal, 2.0)));

    const auto first = buffer.dequeue();
    ASSERT_TRUE(first.has_value());
    EXPECT_STREQ(first->channel_id, "a");
    EXPECT_DOUBLE_EQ(first->value, 1.0);

    const auto second = buffer.dequeue();
    ASSERT_TRUE(second.has_value());
    EXPECT_STREQ(second->channel_id, "b");
}

TEST(RingBufferTest, OverflowDropsLowPriorityFirst) {
    bossa::telemetry::RingBuffer buffer(2);
    ASSERT_TRUE(buffer.enqueue(
        make_sample("low", bossa::telemetry::Priority::kLow, 1.0)));
    ASSERT_TRUE(buffer.enqueue(
        make_sample("normal", bossa::telemetry::Priority::kNormal, 2.0)));
    ASSERT_TRUE(buffer.enqueue(
        make_sample("critical", bossa::telemetry::Priority::kCritical, 3.0)));

    EXPECT_EQ(buffer.size(), 2u);
    EXPECT_EQ(buffer.drop_count(), 1u);

    const auto first = buffer.dequeue();
    ASSERT_TRUE(first.has_value());
    EXPECT_STREQ(first->channel_id, "normal");

    const auto second = buffer.dequeue();
    ASSERT_TRUE(second.has_value());
    EXPECT_STREQ(second->channel_id, "critical");
}
