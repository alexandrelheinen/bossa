/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "bossa/telemetry/channel.hpp"

namespace bossa::core {

/** @brief Edge node identity from configuration. */
struct NodeConfig {
    std::string id;
    std::string api_key_file;
};

/** @brief Remote upload target (Cloudflare Worker URL in Phase 4). */
struct ServerConfig {
    std::string url;
    int upload_timeout_seconds{30};
    int max_batch_size{500};
};

/** @brief Edge SQLite offline buffer settings. */
struct LocalStorageConfig {
    std::filesystem::path path{"/var/lib/bossa/telemetry.db"};
    int max_size_megabytes{256};
    int retention_hours{72};
};

/** @brief Per-channel sync policy from YAML. */
struct ChannelSyncConfig {
    bool destination_local{true};
    bool destination_remote{true};
    int remote_interval_seconds{60};
    telemetry::Priority priority{telemetry::Priority::kNormal};
    telemetry::SyncMode mode{telemetry::SyncMode::kBatch};
    double on_change_threshold{0.0};
};

/** @brief One configured sampling channel. */
struct ChannelConfig {
    std::string id;
    std::string driver;
    nlohmann::json parameters = nlohmann::json::object();
    double sample_rate_hz{1.0};
    ChannelSyncConfig sync;
};

/**
 * @brief Parsed edge configuration.
 */
struct Config {
    int config_version = 0;
    NodeConfig node;
    ServerConfig server;
    LocalStorageConfig local_storage;
    std::vector<ChannelConfig> channels;
};

/**
 * @brief Result of loading a configuration file.
 *
 * @details Uses an explicit success/failure type instead of exceptions so
 * startup errors are visible and testable.
 */
class ConfigLoadResult {
  public:
    static ConfigLoadResult ok(Config config);
    static ConfigLoadResult fail(std::string message);

    /** @brief True when configuration was loaded and validated. */
    bool ok() const;

    /** @brief Loaded configuration; valid only when ok() is true. */
    const Config &value() const;

    /** @brief Human-readable error message; valid only when ok() is false. */
    const std::string &error() const;

  private:
    ConfigLoadResult(bool success, Config config, std::string message);

    bool success_ = false;
    Config config_;
    std::string message_;
};

/**
 * @brief Load and validate a YAML configuration file.
 * @param path Path to the YAML file.
 * @return ConfigLoadResult with config or error message.
 */
ConfigLoadResult load_config(const std::filesystem::path &path);

} // namespace bossa::core
