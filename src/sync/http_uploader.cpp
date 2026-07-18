/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/sync/http_uploader.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>
#include <syslog.h>

namespace bossa::sync {

namespace {

std::string format_time(std::chrono::system_clock::time_point timestamp) {
    const auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::tm utc{};
    gmtime_r(&time, &utc);
    std::ostringstream stream;
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

const char *quality_to_string(telemetry::SampleQuality quality) {
    switch (quality) {
    case telemetry::SampleQuality::kGood:
        return "good";
    case telemetry::SampleQuality::kUncertain:
        return "uncertain";
    case telemetry::SampleQuality::kBad:
        return "bad";
    }
    return "good";
}

std::string join_url(std::string base, const char *path) {
    if (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    return base + path;
}

} // namespace

HttpUploader::HttpUploader(HttpClient &client, storage::LocalStore *store,
                           std::string server_url, std::string node_id,
                           std::string api_key, int timeout_seconds)
    : client_(client), store_(store), server_url_(std::move(server_url)),
      node_id_(std::move(node_id)), api_key_(std::move(api_key)),
      timeout_seconds_(timeout_seconds) {}

std::string HttpUploader::build_payload(
    const std::vector<telemetry::StoredSample> &samples) const {
    nlohmann::json payload;
    payload["node_id"] = node_id_;
    payload["sent_at"] = format_time(std::chrono::system_clock::now());
    payload["samples"] = nlohmann::json::array();
    for (const auto &sample : samples) {
        payload["samples"].push_back({
            {"channel_id", sample.channel_id},
            {"timestamp", format_time(sample.timestamp)},
            {"value", sample.value},
            {"unit", sample.unit},
            {"quality", quality_to_string(sample.quality)},
        });
    }
    return payload.dump();
}

UploadResult
HttpUploader::interpret_response(const HttpResponse &response) const {
    if (!response.error.empty() || response.status_code == 0) {
        return UploadResult::kTransientFailure;
    }
    if (response.status_code == 202 || response.status_code == 200) {
        return UploadResult::kAccepted;
    }
    if (response.status_code == 400) {
        return UploadResult::kRejected;
    }
    if (response.status_code == 401 || response.status_code == 503 ||
        response.status_code >= 500) {
        return UploadResult::kTransientFailure;
    }
    return UploadResult::kTransientFailure;
}

bool HttpUploader::persist_all(
    const std::vector<telemetry::StoredSample> &samples) {
    if (store_ == nullptr) {
        return false;
    }
    bool ok = true;
    for (const auto &sample : samples) {
        if (!store_->enqueue(sample)) {
            syslog(LOG_ERR, "http_uploader: failed to persist sample: %s",
                   store_->last_error().c_str());
            ok = false;
        }
    }
    return ok;
}

UploadResult
HttpUploader::upload(const std::vector<telemetry::StoredSample> &samples) {
    if (samples.empty()) {
        return UploadResult::kAccepted;
    }

    const std::string url = join_url(server_url_, kTelemetryPath);
    const std::string body = build_payload(samples);
    std::map<std::string, std::string> headers;
    if (!api_key_.empty()) {
        headers["Authorization"] = "Bearer " + api_key_;
    }

    const HttpResponse response =
        client_.post(url, headers, body, timeout_seconds_);
    const UploadResult result = interpret_response(response);

    if (result == UploadResult::kAccepted) {
        return result;
    }
    if (result == UploadResult::kRejected) {
        syslog(LOG_ERR, "http_uploader: server rejected batch (%d)",
               response.status_code);
        return result;
    }

    syslog(LOG_WARNING,
           "http_uploader: transient failure (%d %s); persisting %zu samples",
           response.status_code, response.error.c_str(), samples.size());
    persist_all(samples);
    return UploadResult::kTransientFailure;
}

UploadResult HttpUploader::retry_pending(std::size_t max_batch) {
    if (store_ == nullptr) {
        return UploadResult::kAccepted;
    }

    const auto rows = store_->peek_batch(max_batch);
    if (rows.empty()) {
        return UploadResult::kAccepted;
    }

    std::vector<telemetry::StoredSample> samples;
    samples.reserve(rows.size());
    for (const auto &row : rows) {
        samples.push_back(row.sample);
    }

    const std::string url = join_url(server_url_, kTelemetryPath);
    const std::string body = build_payload(samples);
    std::map<std::string, std::string> headers;
    if (!api_key_.empty()) {
        headers["Authorization"] = "Bearer " + api_key_;
    }

    const HttpResponse response =
        client_.post(url, headers, body, timeout_seconds_);
    const UploadResult result = interpret_response(response);
    if (result != UploadResult::kAccepted) {
        return result;
    }

    for (const auto &row : rows) {
        store_->acknowledge(row.id);
    }
    return UploadResult::kAccepted;
}

} // namespace bossa::sync
