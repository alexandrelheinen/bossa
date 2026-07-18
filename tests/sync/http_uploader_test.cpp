/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <filesystem>
#include <map>
#include <vector>

#include <gtest/gtest.h>

#include "bossa/storage/local_store.hpp"
#include "bossa/sync/http_client.hpp"
#include "bossa/sync/http_uploader.hpp"

namespace {

class MockHttpClient : public bossa::sync::HttpClient {
  public:
    std::vector<int> status_sequence;
    std::size_t call_count{0};
    std::string last_url;
    std::string last_body;

    bossa::sync::HttpResponse
    post(const std::string &url,
         const std::map<std::string, std::string> & /*headers*/,
         const std::string &body, int /*timeout_seconds*/) override {
        last_url = url;
        last_body = body;
        bossa::sync::HttpResponse response;
        if (call_count < status_sequence.size()) {
            response.status_code = status_sequence[call_count];
        } else {
            response.status_code = 202;
        }
        ++call_count;
        return response;
    }
};

bossa::telemetry::StoredSample make_sample(double value) {
    bossa::telemetry::Sample sample;
    sample.channel_id = "ambient_temperature";
    sample.unit = "celsius";
    sample.value = value;
    sample.timestamp = std::chrono::system_clock::now();
    return bossa::telemetry::StoredSample::from_sample(
        sample, bossa::telemetry::Priority::kNormal);
}

} // namespace

TEST(HttpUploaderTest, NetworkFailurePersistsThenRetrySucceeds) {
    const auto path =
        std::filesystem::temp_directory_path() / "bossa_uploader_test.db";
    std::filesystem::remove(path);

    bossa::storage::LocalStore store;
    ASSERT_TRUE(store.open(path)) << store.last_error();

    MockHttpClient client;
    client.status_sequence = {503, 202};

    bossa::sync::HttpUploader uploader(
        client, &store, "https://telemetry.example.com", "node-1", "secret", 5);

    const auto result = uploader.upload({make_sample(21.0)});
    EXPECT_EQ(result, bossa::sync::UploadResult::kTransientFailure);
    EXPECT_EQ(store.pending_count(), 1u);
    EXPECT_NE(client.last_url.find("/api/v1/telemetry"), std::string::npos);

    const auto retry = uploader.retry_pending(10);
    EXPECT_EQ(retry, bossa::sync::UploadResult::kAccepted);
    EXPECT_EQ(store.pending_count(), 0u);
    EXPECT_EQ(client.call_count, 2u);

    store.close();
    std::filesystem::remove(path);
}
