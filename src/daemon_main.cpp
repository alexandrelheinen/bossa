/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <syslog.h>

#include "bossa/core/config.hpp"
#include "bossa/core/service.hpp"
#include "bossa/telemetry/scheduler.hpp"

namespace {

constexpr const char *kDefaultConfigPath = "/etc/bossa/config.yaml";

/**
 * @brief Edge service: heartbeat or channel schedule with SIGHUP reload.
 */
class EdgeService : public bossa::core::Service {
  public:
    EdgeService(bossa::core::ServiceOptions options,
                std::filesystem::path config_path, bossa::core::Config config)
        : bossa::core::Service("bossa", options),
          config_path_(std::move(config_path)), config_(std::move(config)) {
        apply_schedule();
    }

    void reload() override {
        const auto result = bossa::core::load_config(config_path_);
        if (!result.ok()) {
            syslog(LOG_ERR, "reload failed: %s", result.error().c_str());
            return;
        }
        config_ = result.value();
        apply_schedule();
        syslog(LOG_INFO, "reload applied (%zu channels)",
               config_.channels.size());
    }

    void loop() override {
        if (scheduler_.channel_count() == 0) {
            syslog(LOG_INFO, "heartbeat");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return;
        }

        scheduler_.dispatch_due([](const std::string &channel_id) {
            syslog(LOG_INFO, "channel due: %s", channel_id.c_str());
        });

        const auto next = scheduler_.next_deadline();
        if (!next.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (*next > now) {
            std::this_thread::sleep_until(*next);
        }
    }

  private:
    void apply_schedule() {
        std::vector<bossa::telemetry::ScheduleEntry> entries;
        entries.reserve(config_.channels.size());
        for (const auto &channel : config_.channels) {
            entries.push_back({channel.id, channel.sample_rate_hz});
        }
        scheduler_.configure(std::move(entries));
    }

    std::filesystem::path config_path_;
    bossa::core::Config config_;
    bossa::telemetry::Scheduler scheduler_;
};

bool parse_args(int argc, char *argv[], bool *foreground,
                std::string *config_path) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--foreground") {
            *foreground = true;
        } else if (arg == "--config") {
            if (i + 1 >= argc) {
                syslog(LOG_ERR, "missing value for --config");
                return false;
            }
            *config_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            syslog(LOG_INFO,
                   "usage: bossa-daemon [--foreground] [--config PATH]");
            return false;
        } else {
            syslog(LOG_ERR, "unknown argument: %s", arg.c_str());
            return false;
        }
    }
    return true;
}

void log_config_error(bool foreground, const std::string &message) {
    syslog(LOG_ERR, "config: %s", message.c_str());
    if (foreground) {
        std::fprintf(stderr, "config: %s\n", message.c_str());
    }
}

} // namespace

int main(int argc, char *argv[]) {
    openlog("bossa", LOG_PID | LOG_CONS, LOG_DAEMON);

    bool foreground = false;
    std::string config_path = kDefaultConfigPath;

    if (!parse_args(argc, argv, &foreground, &config_path)) {
        return EXIT_FAILURE;
    }

    const auto config_result = bossa::core::load_config(config_path);
    if (!config_result.ok()) {
        log_config_error(foreground, config_result.error());
        return EXIT_FAILURE;
    }

    bossa::core::ServiceOptions options;
    options.foreground = foreground;

    EdgeService service(options, config_path, config_result.value());
    return service.start();
}
