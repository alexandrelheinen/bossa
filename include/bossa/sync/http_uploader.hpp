/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "bossa/storage/local_store.hpp"
#include "bossa/sync/http_client.hpp"
#include "bossa/telemetry/stored_sample.hpp"

namespace bossa::sync {

/** @brief Outcome of an upload attempt. */
enum class UploadResult {
    kAccepted,
    kRejected,
    kTransientFailure,
};

/**
 * @brief Posts telemetry batches to server.url (Cloudflare Worker or stub).
 *
 * On transient failure, samples are persisted to LocalStore for retry.
 */
class HttpUploader {
  public:
    /**
     * @brief Construct an uploader.
     * @param client HTTP client implementation.
     * @param store Offline SQLite store (may be null to skip persistence).
     * @param server_url Base server URL (e.g. https://telemetry.example.com).
     * @param node_id Edge node id.
     * @param api_key Bearer token.
     * @param timeout_seconds HTTP timeout.
     */
    HttpUploader(HttpClient &client, storage::LocalStore *store,
                 std::string server_url, std::string node_id,
                 std::string api_key, int timeout_seconds);

    /**
     * @brief Upload a batch of in-memory samples.
     * @param samples Samples to post.
     * @return Upload outcome.
     */
    UploadResult upload(const std::vector<telemetry::StoredSample> &samples);

    /**
     * @brief Retry pending rows from LocalStore.
     * @param max_batch Maximum rows per attempt.
     * @return Upload outcome of the attempt (kAccepted if queue empty).
     */
    UploadResult retry_pending(std::size_t max_batch);

    /** @brief Endpoint path appended to server_url. */
    static constexpr const char *kTelemetryPath = "/api/v1/telemetry";

  private:
    std::string
    build_payload(const std::vector<telemetry::StoredSample> &samples) const;
    UploadResult interpret_response(const HttpResponse &response) const;
    bool persist_all(const std::vector<telemetry::StoredSample> &samples);

    HttpClient &client_;
    storage::LocalStore *store_;
    std::string server_url_;
    std::string node_id_;
    std::string api_key_;
    int timeout_seconds_{30};
};

} // namespace bossa::sync
