/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "bossa/core/config.hpp"

namespace {

std::filesystem::path write_temp_config(const std::string &contents) {
    static std::atomic<int> counter{0};
    const auto path =
        std::filesystem::temp_directory_path() /
        ("bossa_config_test_" + std::to_string(counter.fetch_add(1)) + ".yaml");

    std::ofstream output(path);
    output << contents;
    output.close();
    return path;
}

void remove_if_exists(const std::filesystem::path &path) {
    std::error_code error;
    std::filesystem::remove(path, error);
}

} // namespace

// Phase 1.6 — valid YAML loads node fields.
TEST(ConfigTest, LoadsValidConfig) {
    const auto path = write_temp_config(R"(
config_version: 1
node:
  id: test-node-01
  api_key_file: /etc/bossa/api.key
channels: []
)");

    const auto result = bossa::core::load_config(path);
    ASSERT_TRUE(result.ok()) << result.error();

    EXPECT_EQ(result.value().config_version, 1);
    EXPECT_EQ(result.value().node.id, "test-node-01");
    EXPECT_EQ(result.value().node.api_key_file, "/etc/bossa/api.key");

    remove_if_exists(path);
}

// Phase 1 acceptance — invalid YAML returns an error.
TEST(ConfigTest, RejectsInvalidYaml) {
    const auto path = write_temp_config("config_version: [not: valid: yaml");

    const auto result = bossa::core::load_config(path);
    EXPECT_FALSE(result.ok());
    EXPECT_FALSE(result.error().empty());

    remove_if_exists(path);
}

// Phase 1 acceptance — unsupported config_version is rejected.
TEST(ConfigTest, RejectsUnsupportedVersion) {
    const auto path = write_temp_config(R"(
config_version: 99
node:
  id: test-node
  api_key_file: /etc/bossa/api.key
)");

    const auto result = bossa::core::load_config(path);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error().find("unsupported config_version"),
              std::string::npos);

    remove_if_exists(path);
}

// Defensive — missing node.id is rejected (no silent default).
TEST(ConfigTest, RejectsEmptyNodeId) {
    const auto path = write_temp_config(R"(
config_version: 1
node:
  id: ""
  api_key_file: /etc/bossa/api.key
)");

    const auto result = bossa::core::load_config(path);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error().find("node.id"), std::string::npos);

    remove_if_exists(path);
}

// Defensive — missing file returns a clear error.
TEST(ConfigTest, RejectsMissingFile) {
    const auto path =
        std::filesystem::temp_directory_path() / "bossa_missing_config.yaml";

    const auto result = bossa::core::load_config(path);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error().find("not found"), std::string::npos);
}
