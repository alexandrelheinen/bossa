/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <thread>

#include <gtest/gtest.h>

#include "bossa/core/service.hpp"

namespace {

class CountingService : public bossa::core::Service {
  public:
    explicit CountingService(bossa::core::ServiceOptions options = {})
        : bossa::core::Service("test-service", options) {}

    void loop() override { loop_count_.fetch_add(1); }

    int loop_count() const { return loop_count_.load(); }

  private:
    std::atomic<int> loop_count_{0};
};

} // namespace

// FR-CFG-01 / Phase 1.3 — Service runs loop in foreground mode.
TEST(ServiceTest, ForegroundModeInvokesLoop) {
    bossa::core::ServiceOptions options;
    options.foreground = true;

    CountingService service(options);
    std::thread worker([&service]() { service.start(); });

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (service.loop_count() < 2 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    bossa::core::Service::request_stop();
    worker.join();

    EXPECT_GE(service.loop_count(), 2);
}

// Phase 1.3 — request_stop clears the running flag.
TEST(ServiceTest, RequestStopEndsLoop) {
    bossa::core::ServiceOptions options;
    options.foreground = true;

    CountingService service(options);
    EXPECT_TRUE(bossa::core::Service::is_running());

    std::thread worker([&service]() { service.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    bossa::core::Service::request_stop();
    worker.join();

    EXPECT_FALSE(bossa::core::Service::is_running());
}

// Phase 1.5 — SIGTERM sets the stop flag (sigaction handler).
TEST(ServiceTest, SigtermStopsService) {
    bossa::core::ServiceOptions options;
    options.foreground = true;

    CountingService service(options);
    std::thread worker([&service]() { service.start(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(std::raise(SIGTERM), 0);

    worker.join();
    EXPECT_FALSE(bossa::core::Service::is_running());
}

// Phase 3.9 — SIGHUP invokes reload().
TEST(ServiceTest, SighupInvokesReload) {
    class ReloadService : public bossa::core::Service {
      public:
        ReloadService()
            : bossa::core::Service("reload-service",
                                   bossa::core::ServiceOptions{true}) {}
        void loop() override {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        void reload() override { reload_count_.fetch_add(1); }
        int reload_count() const { return reload_count_.load(); }

      private:
        std::atomic<int> reload_count_{0};
    };

    ReloadService service;
    std::thread worker([&service]() { service.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    ASSERT_EQ(std::raise(SIGHUP), 0);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (service.reload_count() < 1 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    bossa::core::Service::request_stop();
    worker.join();
    EXPECT_GE(service.reload_count(), 1);
}

// Phase 1.4 — foreground services do not require daemon parent exit.
TEST(ServiceTest, ForegroundStartReturnsSuccess) {
    bossa::core::ServiceOptions options;
    options.foreground = true;

    CountingService service(options);
    std::thread worker(
        [&service]() { EXPECT_EQ(service.start(), EXIT_SUCCESS); });

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    bossa::core::Service::request_stop();
    worker.join();
}
