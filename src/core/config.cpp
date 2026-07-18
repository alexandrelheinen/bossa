/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/core/config.hpp"

#include <fstream>
#include <optional>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace bossa::core {

namespace {

constexpr int kSupportedConfigVersion = 1;
constexpr std::size_t kMaxConfigBytes = 1024 * 1024;

bool is_non_empty_string(const YAML::Node &node) {
    return node && node.IsScalar() && !node.as<std::string>().empty();
}

std::optional<telemetry::Priority> parse_priority(const std::string &text) {
    if (text == "low") {
        return telemetry::Priority::kLow;
    }
    if (text == "normal") {
        return telemetry::Priority::kNormal;
    }
    if (text == "critical") {
        return telemetry::Priority::kCritical;
    }
    return std::nullopt;
}

std::optional<telemetry::SyncMode> parse_sync_mode(const std::string &text) {
    if (text == "batch") {
        return telemetry::SyncMode::kBatch;
    }
    if (text == "realtime") {
        return telemetry::SyncMode::kRealtime;
    }
    if (text == "on_change") {
        return telemetry::SyncMode::kOnChange;
    }
    return std::nullopt;
}

nlohmann::json yaml_to_json(const YAML::Node &node) {
    if (!node || node.IsNull()) {
        return nullptr;
    }
    if (node.IsScalar()) {
        try {
            return node.as<bool>();
        } catch (...) {
        }
        try {
            return node.as<int>();
        } catch (...) {
        }
        try {
            return node.as<double>();
        } catch (...) {
        }
        return node.as<std::string>();
    }
    if (node.IsSequence()) {
        nlohmann::json array = nlohmann::json::array();
        for (const auto &item : node) {
            array.push_back(yaml_to_json(item));
        }
        return array;
    }
    if (node.IsMap()) {
        nlohmann::json object = nlohmann::json::object();
        for (const auto &item : node) {
            object[item.first.as<std::string>()] = yaml_to_json(item.second);
        }
        return object;
    }
    return nullptr;
}

ConfigLoadResult parse_server(const YAML::Node &root, Config &config) {
    if (!root["server"]) {
        return ConfigLoadResult::ok(config);
    }
    if (!root["server"].IsMap()) {
        return ConfigLoadResult::fail("server must be a mapping");
    }
    const YAML::Node server = root["server"];
    if (server["url"]) {
        if (!is_non_empty_string(server["url"])) {
            return ConfigLoadResult::fail(
                "server.url must be a non-empty string");
        }
        config.server.url = server["url"].as<std::string>();
    }
    if (server["upload_timeout_seconds"]) {
        config.server.upload_timeout_seconds =
            server["upload_timeout_seconds"].as<int>();
        if (config.server.upload_timeout_seconds <= 0) {
            return ConfigLoadResult::fail(
                "server.upload_timeout_seconds must be > 0");
        }
    }
    if (server["max_batch_size"]) {
        config.server.max_batch_size = server["max_batch_size"].as<int>();
        if (config.server.max_batch_size <= 0) {
            return ConfigLoadResult::fail("server.max_batch_size must be > 0");
        }
    }
    return ConfigLoadResult::ok(config);
}

ConfigLoadResult parse_local_storage(const YAML::Node &root, Config &config) {
    if (!root["local_storage"]) {
        return ConfigLoadResult::ok(config);
    }
    if (!root["local_storage"].IsMap()) {
        return ConfigLoadResult::fail("local_storage must be a mapping");
    }
    const YAML::Node storage = root["local_storage"];
    if (storage["path"]) {
        if (!is_non_empty_string(storage["path"])) {
            return ConfigLoadResult::fail(
                "local_storage.path must be a non-empty string");
        }
        config.local_storage.path = storage["path"].as<std::string>();
    }
    if (storage["max_size_megabytes"]) {
        config.local_storage.max_size_megabytes =
            storage["max_size_megabytes"].as<int>();
        if (config.local_storage.max_size_megabytes <= 0) {
            return ConfigLoadResult::fail(
                "local_storage.max_size_megabytes must be > 0");
        }
    }
    if (storage["retention_hours"]) {
        config.local_storage.retention_hours =
            storage["retention_hours"].as<int>();
        if (config.local_storage.retention_hours <= 0) {
            return ConfigLoadResult::fail(
                "local_storage.retention_hours must be > 0");
        }
    }
    return ConfigLoadResult::ok(config);
}

ConfigLoadResult parse_channels(const YAML::Node &root, Config &config) {
    if (!root["channels"]) {
        return ConfigLoadResult::ok(config);
    }
    if (!root["channels"].IsSequence()) {
        return ConfigLoadResult::fail("channels must be a sequence");
    }

    for (std::size_t index = 0; index < root["channels"].size(); ++index) {
        const YAML::Node node = root["channels"][index];
        if (!node.IsMap()) {
            return ConfigLoadResult::fail("channels[" + std::to_string(index) +
                                          "] must be a mapping");
        }
        if (!is_non_empty_string(node["id"])) {
            return ConfigLoadResult::fail("channels[" + std::to_string(index) +
                                          "].id must be a non-empty string");
        }
        if (!is_non_empty_string(node["driver"])) {
            return ConfigLoadResult::fail(
                "channels[" + std::to_string(index) +
                "].driver must be a non-empty string");
        }
        if (!node["sample_rate_hz"]) {
            return ConfigLoadResult::fail("channels[" + std::to_string(index) +
                                          "].sample_rate_hz is required");
        }

        ChannelConfig channel;
        channel.id = node["id"].as<std::string>();
        channel.driver = node["driver"].as<std::string>();
        channel.sample_rate_hz = node["sample_rate_hz"].as<double>();
        if (!(channel.sample_rate_hz > 0.0)) {
            return ConfigLoadResult::fail("channels[" + std::to_string(index) +
                                          "].sample_rate_hz must be > 0");
        }
        if (node["parameters"]) {
            channel.parameters = yaml_to_json(node["parameters"]);
        }

        if (node["sync"]) {
            if (!node["sync"].IsMap()) {
                return ConfigLoadResult::fail("channels[" +
                                              std::to_string(index) +
                                              "].sync must be a mapping");
            }
            const YAML::Node sync = node["sync"];
            if (sync["destinations"]) {
                if (!sync["destinations"].IsSequence()) {
                    return ConfigLoadResult::fail(
                        "channels[" + std::to_string(index) +
                        "].sync.destinations must be a sequence");
                }
                channel.sync.destination_local = false;
                channel.sync.destination_remote = false;
                for (const auto &destination : sync["destinations"]) {
                    const std::string value = destination.as<std::string>();
                    if (value == "local") {
                        channel.sync.destination_local = true;
                    } else if (value == "remote") {
                        channel.sync.destination_remote = true;
                    } else {
                        return ConfigLoadResult::fail(
                            "channels[" + std::to_string(index) +
                            "].sync.destinations contains unknown value");
                    }
                }
            }
            if (sync["remote_interval_seconds"]) {
                channel.sync.remote_interval_seconds =
                    sync["remote_interval_seconds"].as<int>();
                if (channel.sync.remote_interval_seconds <= 0) {
                    return ConfigLoadResult::fail(
                        "channels[" + std::to_string(index) +
                        "].sync.remote_interval_seconds must be > 0");
                }
            }
            if (sync["priority"]) {
                const auto priority =
                    parse_priority(sync["priority"].as<std::string>());
                if (!priority.has_value()) {
                    return ConfigLoadResult::fail(
                        "channels[" + std::to_string(index) +
                        "].sync.priority must be low|normal|critical");
                }
                channel.sync.priority = *priority;
            }
            if (sync["mode"]) {
                const auto mode =
                    parse_sync_mode(sync["mode"].as<std::string>());
                if (!mode.has_value()) {
                    return ConfigLoadResult::fail(
                        "channels[" + std::to_string(index) +
                        "].sync.mode must be batch|realtime|on_change");
                }
                channel.sync.mode = *mode;
            }
            if (sync["on_change_threshold"]) {
                channel.sync.on_change_threshold =
                    sync["on_change_threshold"].as<double>();
            }
        }

        config.channels.push_back(std::move(channel));
    }

    return ConfigLoadResult::ok(config);
}

} // namespace

ConfigLoadResult::ConfigLoadResult(bool success, Config config,
                                   std::string message)
    : success_(success), config_(std::move(config)),
      message_(std::move(message)) {}

ConfigLoadResult ConfigLoadResult::ok(Config config) {
    return ConfigLoadResult(true, std::move(config), {});
}

ConfigLoadResult ConfigLoadResult::fail(std::string message) {
    return ConfigLoadResult(false, {}, std::move(message));
}

bool ConfigLoadResult::ok() const { return success_; }

const Config &ConfigLoadResult::value() const { return config_; }

const std::string &ConfigLoadResult::error() const { return message_; }

ConfigLoadResult load_config(const std::filesystem::path &path) {
    if (path.empty()) {
        return ConfigLoadResult::fail("config path is empty");
    }

    if (!std::filesystem::exists(path)) {
        return ConfigLoadResult::fail("config file not found: " +
                                      path.string());
    }

    std::error_code size_error;
    const auto file_size = std::filesystem::file_size(path, size_error);
    if (size_error) {
        return ConfigLoadResult::fail("cannot read config file size: " +
                                      path.string());
    }
    if (file_size > kMaxConfigBytes) {
        return ConfigLoadResult::fail("config file exceeds size limit: " +
                                      path.string());
    }

    std::ifstream input(path);
    if (!input.is_open()) {
        return ConfigLoadResult::fail("cannot open config file: " +
                                      path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        return ConfigLoadResult::fail("error reading config file: " +
                                      path.string());
    }

    YAML::Node root;
    try {
        root = YAML::Load(buffer.str());
    } catch (const YAML::Exception &ex) {
        return ConfigLoadResult::fail(std::string("YAML parse error: ") +
                                      ex.what());
    }

    if (!root || !root.IsMap()) {
        return ConfigLoadResult::fail("config root must be a mapping");
    }

    if (!root["config_version"]) {
        return ConfigLoadResult::fail("missing required field: config_version");
    }

    const int config_version = root["config_version"].as<int>();
    if (config_version != kSupportedConfigVersion) {
        return ConfigLoadResult::fail("unsupported config_version: " +
                                      std::to_string(config_version));
    }

    if (!root["node"] || !root["node"].IsMap()) {
        return ConfigLoadResult::fail("missing required field: node");
    }

    const YAML::Node node = root["node"];
    if (!is_non_empty_string(node["id"])) {
        return ConfigLoadResult::fail("node.id must be a non-empty string");
    }
    if (!is_non_empty_string(node["api_key_file"])) {
        return ConfigLoadResult::fail(
            "node.api_key_file must be a non-empty string");
    }

    Config config;
    config.config_version = config_version;
    config.node.id = node["id"].as<std::string>();
    config.node.api_key_file = node["api_key_file"].as<std::string>();

    auto server_result = parse_server(root, config);
    if (!server_result.ok()) {
        return server_result;
    }
    config = server_result.value();

    auto storage_result = parse_local_storage(root, config);
    if (!storage_result.ok()) {
        return storage_result;
    }
    config = storage_result.value();

    auto channels_result = parse_channels(root, config);
    if (!channels_result.ok()) {
        return channels_result;
    }
    return channels_result;
}

} // namespace bossa::core
