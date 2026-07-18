/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/runtime/telemetry_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <syslog.h>

#include "bossa/drivers/registry.hpp"

// Pull driver TUs that only register via static initializers.
extern void bossa_force_link_sim_driver();
extern void bossa_force_link_bme280_driver();
namespace {
using ForceLinkFn = void (*)();
constexpr ForceLinkFn kForceLinkDrivers[] = {
    &bossa_force_link_sim_driver,
    &bossa_force_link_bme280_driver,
};
} // namespace

namespace bossa::runtime {

namespace {

std::string read_api_key_file(const std::filesystem::path &path) {
    if (path.empty()) {
        return {};
    }
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }
    std::string key;
    std::getline(input, key);
    while (!key.empty() && (key.back() == '\n' || key.back() == '\r')) {
        key.pop_back();
    }
    return key;
}

} // namespace

TelemetryRuntime::TelemetryRuntime(sync::HttpClient &client) : client_(client) {
    // Call once so the linker keeps driver object files (static registrars).
    for (auto *force : kForceLinkDrivers) {
        force();
    }
}

bool TelemetryRuntime::configure(const core::Config &config,
                                 std::string api_key) {
    last_error_.clear();
    config_ = config;
    if (!api_key.empty()) {
        api_key_ = std::move(api_key);
    } else {
        api_key_ = read_api_key_file(config_.node.api_key_file);
    }

    channels_.clear();
    policy_.clear();
    uploader_.reset();
    buffer_.reset();
    accepted_upload_count_ = 0;
    reset_backoff_after_success();

    const std::size_t capacity =
        config_.server.max_batch_size > 0
            ? static_cast<std::size_t>(config_.server.max_batch_size)
            : 500;
    buffer_ = std::make_unique<telemetry::RingBuffer>(capacity);

    if (!config_.local_storage.path.empty()) {
        store_open_ = store_.open(config_.local_storage.path);
        if (!store_open_) {
            last_error_ = store_.last_error();
            syslog(LOG_ERR, "telemetry: local store open failed: %s",
                   last_error_.c_str());
            return false;
        }
    } else {
        store_open_ = false;
    }

    std::vector<telemetry::ScheduleEntry> schedule;
    schedule.reserve(config_.channels.size());

    for (const auto &channel_config : config_.channels) {
        if (channel_config.sample_rate_hz <= 0.0) {
            continue;
        }
        if (!drivers::Registry::contains(channel_config.driver)) {
            last_error_ = "unknown driver: " + channel_config.driver;
            syslog(LOG_ERR, "telemetry: %s", last_error_.c_str());
            return false;
        }

        auto driver = drivers::Registry::create(channel_config.driver);
        try {
            driver->configure(channel_config.parameters);
        } catch (const std::exception &ex) {
            last_error_ = std::string("configure ") + channel_config.id + ": " +
                          ex.what();
            syslog(LOG_ERR, "telemetry: %s", last_error_.c_str());
            return false;
        }

        sync::ChannelSyncState sync_state;
        sync_state.channel_id = channel_config.id;
        sync_state.mode = channel_config.sync.mode;
        sync_state.priority = channel_config.sync.priority;
        sync_state.remote_interval =
            std::chrono::seconds(channel_config.sync.remote_interval_seconds);
        sync_state.on_change_threshold =
            channel_config.sync.on_change_threshold;
        sync_state.destination_remote = channel_config.sync.destination_remote;
        policy_.configure_channel(std::move(sync_state));

        schedule.push_back({channel_config.id, channel_config.sample_rate_hz});

        ChannelBinding binding;
        binding.config = channel_config;
        binding.driver = std::move(driver);
        channels_.emplace(channel_config.id, std::move(binding));
    }

    scheduler_.configure(std::move(schedule));

    if (!config_.server.url.empty()) {
        uploader_ = std::make_unique<sync::HttpUploader>(
            client_, store_open_ ? &store_ : nullptr, config_.server.url,
            config_.node.id, api_key_, config_.server.upload_timeout_seconds);
    }

    syslog(LOG_INFO, "telemetry: configured %zu channels node=%s",
           channels_.size(), config_.node.id.c_str());
    return true;
}

void TelemetryRuntime::poll_channel(const std::string &channel_id) {
    auto it = channels_.find(channel_id);
    if (it == channels_.end() || !it->second.driver) {
        return;
    }

    const auto result = it->second.driver->read();
    const auto now = Clock::now();
    for (std::size_t i = 0; i < result.sample_count; ++i) {
        const auto &sample = result.samples[i];
        const auto stored = telemetry::StoredSample::from_sample(
            sample, it->second.config.sync.priority);
        buffer_->enqueue(stored);
        policy_.on_sample(channel_id, sample.value, now);
        syslog(LOG_INFO, "telemetry: sample %s=%.3f %.*s", channel_id.c_str(),
               sample.value, static_cast<int>(sample.unit.size()),
               sample.unit.data());
    }
}

void TelemetryRuntime::advance_backoff_after_failure() {
    ++retry_attempt_;
    next_retry_at_ = Clock::now() + backoff_delay(retry_attempt_);
}

void TelemetryRuntime::reset_backoff_after_success() {
    retry_attempt_ = 0;
    next_retry_at_ = TimePoint{};
}

TelemetryRuntime::Clock::duration TelemetryRuntime::backoff_delay(int attempt) {
    // 5s → 10s → 30s → 60s → 300s (cap), matching specification §9.4.
    static constexpr int kDelaysSeconds[] = {5, 10, 30, 60, 300};
    const int index = std::clamp(
        attempt - 1, 0, static_cast<int>(std::size(kDelaysSeconds)) - 1);
    return std::chrono::seconds(kDelaysSeconds[index]);
}

void TelemetryRuntime::maybe_flush() {
    if (!uploader_ || !buffer_) {
        return;
    }
    const auto now = Clock::now();
    const std::size_t buffered = buffer_->size();
    const std::size_t max_batch =
        static_cast<std::size_t>(std::max(1, config_.server.max_batch_size));
    if (!policy_.should_flush(now, buffered, max_batch)) {
        return;
    }

    std::vector<telemetry::StoredSample> batch;
    batch.reserve(std::min(buffered, max_batch));
    while (batch.size() < max_batch) {
        auto sample = buffer_->dequeue();
        if (!sample.has_value()) {
            break;
        }
        batch.push_back(*sample);
    }
    if (batch.empty()) {
        return;
    }

    const auto result = uploader_->upload(batch);
    if (result == sync::UploadResult::kAccepted) {
        policy_.mark_flushed(now);
        reset_backoff_after_success();
        ++accepted_upload_count_;
        return;
    }
    if (result == sync::UploadResult::kRejected) {
        policy_.mark_flushed(now);
        return;
    }
    advance_backoff_after_failure();
}

void TelemetryRuntime::maybe_retry() {
    if (!uploader_ || !store_open_) {
        return;
    }
    if (store_.pending_count() == 0) {
        return;
    }
    const auto now = Clock::now();
    if (retry_attempt_ > 0 && now < next_retry_at_) {
        return;
    }

    const std::size_t max_batch =
        static_cast<std::size_t>(std::max(1, config_.server.max_batch_size));
    const auto result = uploader_->retry_pending(max_batch);
    if (result == sync::UploadResult::kAccepted) {
        reset_backoff_after_success();
        ++accepted_upload_count_;
        return;
    }
    advance_backoff_after_failure();
}

void TelemetryRuntime::tick() {
    if (channels_.empty()) {
        return;
    }
    scheduler_.dispatch_due(
        [this](const std::string &channel_id) { poll_channel(channel_id); });
    maybe_flush();
    maybe_retry();
}

std::optional<TelemetryRuntime::TimePoint> TelemetryRuntime::next_wake() const {
    std::optional<TimePoint> wake = scheduler_.next_deadline();
    if (store_open_ && store_.pending_count() > 0 && retry_attempt_ > 0) {
        if (!wake.has_value() || next_retry_at_ < *wake) {
            wake = next_retry_at_;
        }
    }
    return wake;
}

std::size_t TelemetryRuntime::buffered_count() const {
    return buffer_ ? buffer_->size() : 0;
}

std::size_t TelemetryRuntime::pending_count() const {
    return store_open_ ? store_.pending_count() : 0;
}

std::size_t TelemetryRuntime::accepted_upload_count() const {
    return accepted_upload_count_;
}

const std::string &TelemetryRuntime::last_error() const { return last_error_; }

void TelemetryRuntime::unlock_retry_for_test() {
    retry_attempt_ = 1;
    next_retry_at_ = Clock::now();
}

} // namespace bossa::runtime
