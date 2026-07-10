/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/core/config.hpp"

#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace bossa::core {

namespace {

constexpr int kSupportedConfigVersion = 1;
constexpr std::size_t kMaxConfigBytes = 1024 * 1024;

bool is_non_empty_string(const YAML::Node &node) {
    return node && node.IsScalar() && !node.as<std::string>().empty();
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

    return ConfigLoadResult::ok(std::move(config));
}

} // namespace bossa::core
