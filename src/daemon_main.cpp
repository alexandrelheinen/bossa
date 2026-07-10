/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <syslog.h>
#include <thread>

#include "bossa/core/config.hpp"
#include "bossa/core/service.hpp"

namespace {

constexpr const char *kDefaultConfigPath = "/etc/bossa/config.yaml";

/**
 * @brief Sample service that logs a heartbeat every five seconds.
 */
class SampleService : public bossa::core::Service {
  public:
    SampleService(bossa::core::ServiceOptions options)
        : bossa::core::Service("bossa", options) {}

    void loop() override {
        syslog(LOG_INFO, "heartbeat");
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
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
            syslog(LOG_INFO, "usage: bossa [--foreground] [--config PATH]");
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

    const bossa::core::Config &config = config_result.value();
    syslog(LOG_INFO, "loaded config for node '%s'", config.node.id.c_str());

    bossa::core::ServiceOptions options;
    options.foreground = foreground;

    SampleService service(options);
    return service.start();
}
