/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "bossa/core/test_identity.hpp"
#include "bossa/runtime/telemetry_runtime.hpp"
#include "bossa/sync/http_client.hpp"

namespace {

class MockHttpClient : public bossa::sync::HttpClient {
  public:
    std::vector<int> status_sequence;
    std::size_t call_count{0};
    std::string last_body;

    bossa::sync::HttpResponse
    post(const std::string & /*url*/,
         const std::map<std::string, std::string> & /*headers*/,
         const std::string &body, int /*timeout_seconds*/) override {
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

bossa::core::Config make_sim_config(const std::filesystem::path &db_path) {
    bossa::core::Config config;
    config.config_version = 1;
    config.node.id = bossa::core::kReservedTestNodeId;
    config.node.api_key_file = "/dev/null";
    config.server.url = "https://telemetry.test.local";
    config.server.max_batch_size = 10;
    config.server.upload_timeout_seconds = 5;
    config.local_storage.path = db_path;

    bossa::core::ChannelConfig channel;
    channel.id = "sim_temperature";
    channel.driver = "sim";
    channel.sample_rate_hz = 100.0; // fast for tests
    channel.parameters = {{"channel_id", "sim_temperature"},
                          {"unit", "celsius"},
                          {"base_value", 20.0},
                          {"step", 0.5}};
    channel.sync.destination_local = true;
    channel.sync.destination_remote = true;
    channel.sync.remote_interval_seconds = 1;
    channel.sync.mode = bossa::telemetry::SyncMode::kRealtime;
    channel.sync.priority = bossa::telemetry::Priority::kNormal;
    config.channels.push_back(channel);
    return config;
}

} // namespace

TEST(TelemetryPipelineTest, SimDriverUploadsThroughMiddleware) {
    const auto db_path = std::filesystem::temp_directory_path() /
                         "bossa_pipeline_upload_test.db";
    std::filesystem::remove(db_path);

    MockHttpClient client;
    client.status_sequence = {202};

    bossa::runtime::TelemetryRuntime runtime(client);
    ASSERT_TRUE(runtime.configure(make_sim_config(db_path), "test-key"))
        << runtime.last_error();

    for (int i = 0; i < 20 && runtime.accepted_upload_count() == 0; ++i) {
        runtime.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_GE(runtime.accepted_upload_count(), 1u);
    EXPECT_NE(client.last_body.find(bossa::core::kReservedTestNodeId),
              std::string::npos);
    EXPECT_NE(client.last_body.find("sim_temperature"), std::string::npos);

    std::filesystem::remove(db_path);
}

TEST(TelemetryPipelineTest, NetworkFailureQueuesThenRetries) {
    const auto db_path =
        std::filesystem::temp_directory_path() / "bossa_pipeline_retry_test.db";
    std::filesystem::remove(db_path);

    MockHttpClient client;
    client.status_sequence = {503, 202};

    bossa::runtime::TelemetryRuntime runtime(client);
    ASSERT_TRUE(runtime.configure(make_sim_config(db_path), "test-key"))
        << runtime.last_error();

    for (int i = 0; i < 30 && runtime.pending_count() == 0; ++i) {
        runtime.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_GE(runtime.pending_count(), 1u);
    EXPECT_GE(client.call_count, 1u);

    runtime.unlock_retry_for_test();
    for (int i = 0; i < 20 && runtime.pending_count() > 0; ++i) {
        runtime.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(runtime.pending_count(), 0u);
    EXPECT_GE(client.call_count, 2u);

    std::filesystem::remove(db_path);
}

TEST(TestIdentityTest, ReservedNodeIdIsStable) {
    EXPECT_TRUE(
        bossa::core::is_reserved_test_node(bossa::core::kReservedTestNodeId));
    EXPECT_FALSE(bossa::core::is_reserved_test_node("greenhouse-sensor-01"));
    EXPECT_STREQ(bossa::core::kReservedTestNodeId, "bossa-test-00000000");
}
