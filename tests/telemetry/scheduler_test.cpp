/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <map>
#include <vector>

#include <gtest/gtest.h>

#include "bossa/telemetry/scheduler.hpp"

TEST(SchedulerTest, DispatchesThreeChannelsAtConfiguredRates) {
    using clock = bossa::telemetry::Scheduler::Clock;
    clock::time_point now = clock::time_point{std::chrono::seconds(0)};

    bossa::telemetry::Scheduler scheduler([&now]() { return now; });
    scheduler.configure({
        {"fast", 10.0},  // every 100 ms
        {"medium", 5.0}, // every 200 ms
        {"slow", 2.0},   // every 500 ms
    });

    std::map<std::string, std::vector<clock::time_point>> hits;

    // Initial configure marks all due immediately.
    scheduler.dispatch_due(
        [&](const std::string &id) { hits[id].push_back(now); });

    // Simulate 1 second in 10 ms steps.
    for (int step = 1; step <= 100; ++step) {
        now = clock::time_point{std::chrono::milliseconds(step * 10)};
        scheduler.dispatch_due(
            [&](const std::string &id) { hits[id].push_back(now); });
    }

    ASSERT_GE(hits["fast"].size(), 10u);
    ASSERT_GE(hits["medium"].size(), 5u);
    ASSERT_GE(hits["slow"].size(), 2u);

    auto check_period = [](const std::vector<clock::time_point> &times,
                           int expected_ms) {
        ASSERT_GE(times.size(), 2u);
        for (std::size_t i = 1; i < times.size(); ++i) {
            const auto delta =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    times[i] - times[i - 1])
                    .count();
            EXPECT_NEAR(delta, expected_ms, 10);
        }
    };

    check_period(hits["fast"], 100);
    check_period(hits["medium"], 200);
    check_period(hits["slow"], 500);
}

TEST(SchedulerTest, ReloadChangesSampleRate) {
    using clock = bossa::telemetry::Scheduler::Clock;
    clock::time_point now = clock::time_point{std::chrono::seconds(0)};
    bossa::telemetry::Scheduler scheduler([&now]() { return now; });

    scheduler.configure({{"temp", 1.0}});
    std::vector<clock::time_point> hits;
    scheduler.dispatch_due([&](const std::string &) { hits.push_back(now); });

    now += std::chrono::milliseconds(1000);
    scheduler.dispatch_due([&](const std::string &) { hits.push_back(now); });
    ASSERT_EQ(hits.size(), 2u);

    // SIGHUP path: reconfigure with 10 Hz.
    scheduler.configure({{"temp", 10.0}});
    hits.clear();
    scheduler.dispatch_due([&](const std::string &) { hits.push_back(now); });
    now += std::chrono::milliseconds(100);
    scheduler.dispatch_due([&](const std::string &) { hits.push_back(now); });
    now += std::chrono::milliseconds(100);
    scheduler.dispatch_due([&](const std::string &) { hits.push_back(now); });

    ASSERT_EQ(hits.size(), 3u);
    EXPECT_EQ(
        std::chrono::duration_cast<std::chrono::milliseconds>(hits[1] - hits[0])
            .count(),
        100);
}
