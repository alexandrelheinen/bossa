/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "bossa/core/config.hpp"
#include "bossa/drivers/driver.hpp"
#include "bossa/storage/local_store.hpp"
#include "bossa/sync/http_client.hpp"
#include "bossa/sync/http_uploader.hpp"
#include "bossa/sync/upload_policy.hpp"
#include "bossa/telemetry/ring_buffer.hpp"
#include "bossa/telemetry/scheduler.hpp"

namespace bossa::runtime {

/**
 * @brief Wires config → drivers → scheduler → ring buffer → sync/upload.
 *
 * Hot sampling path avoids heap growth after configure(). Upload/retry may
 * allocate (JSON, SQLite) off the sensor-read critical path.
 */
class TelemetryRuntime {
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    /**
     * @brief Construct a runtime.
     * @param client HTTP client used for remote uploads (owned by caller).
     */
    explicit TelemetryRuntime(sync::HttpClient &client);

    /**
     * @brief (Re)build drivers, schedule, buffer, store, and uploader.
     * @param config Parsed edge configuration.
     * @param api_key Bearer token (may be empty in tests).
     * @return true on success.
     */
    bool configure(const core::Config &config, std::string api_key);

    /**
     * @brief One pipeline step: poll due channels, maybe flush/upload/retry.
     */
    void tick();

    /**
     * @brief Next wake time for the service loop (schedule or retry).
     * @return Absolute steady_clock time, or nullopt when idle/heartbeat.
     */
    std::optional<TimePoint> next_wake() const;

    /** @brief Samples currently buffered for upload. */
    std::size_t buffered_count() const;

    /** @brief Pending rows in the offline SQLite store. */
    std::size_t pending_count() const;

    /** @brief Number of successful remote upload batches. */
    std::size_t accepted_upload_count() const;

    /** @brief Last configure/runtime error message. */
    const std::string &last_error() const;

    /**
     * @brief Test helper — make offline retry eligible on the next tick().
     */
    void unlock_retry_for_test();

  private:
    struct ChannelBinding {
        core::ChannelConfig config;
        std::unique_ptr<drivers::Driver> driver;
    };

    void poll_channel(const std::string &channel_id);
    void maybe_flush();
    void maybe_retry();
    void advance_backoff_after_failure();
    void reset_backoff_after_success();
    static Clock::duration backoff_delay(int attempt);

    sync::HttpClient &client_;
    core::Config config_{};
    std::string api_key_;
    std::unordered_map<std::string, ChannelBinding> channels_;
    telemetry::Scheduler scheduler_;
    std::unique_ptr<telemetry::RingBuffer> buffer_;
    storage::LocalStore store_;
    sync::UploadPolicy policy_;
    std::unique_ptr<sync::HttpUploader> uploader_;
    std::string last_error_;
    std::size_t accepted_upload_count_{0};
    int retry_attempt_{0};
    TimePoint next_retry_at_{};
    bool store_open_{false};
};

} // namespace bossa::runtime
