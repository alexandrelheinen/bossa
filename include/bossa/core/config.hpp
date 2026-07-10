/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <string>

namespace bossa::core {

/** @brief Edge node identity from configuration. */
struct NodeConfig {
    std::string id;
    std::string api_key_file;
};

/**
 * @brief Parsed edge configuration (Phase 1: top-level fields only).
 *
 * @details Channel and sync parsing is added in later phases.
 */
struct Config {
    int config_version = 0;
    NodeConfig node;
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
