/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include <syslog.h>

#include <nlohmann/json.hpp>

#include "bossa/drivers/registry.hpp"

namespace {

constexpr std::string_view kToolName = "bossa-bme280-smoke";
constexpr const char *kDefaultBus = "/dev/i2c-1";
constexpr std::uint8_t kDefaultAddress = 0x76;

void print_usage() {
    std::fprintf(
        stderr,
        "usage: %.*s [--foreground] [--bus PATH] [--address 0x76|0x77] "
        "[--interval-seconds N]\n",
        static_cast<int>(kToolName.size()), kToolName.data());
}

bool parse_args(int argc, char *argv[], bool *foreground, std::string *bus_path,
                std::uint8_t *address, int *interval_seconds) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--foreground") {
            *foreground = true;
        } else if (arg == "--bus") {
            if (i + 1 >= argc) {
                return false;
            }
            *bus_path = argv[++i];
        } else if (arg == "--address") {
            if (i + 1 >= argc) {
                return false;
            }
            *address =
                static_cast<std::uint8_t>(std::stoul(argv[++i], nullptr, 0));
        } else if (arg == "--interval-seconds") {
            if (i + 1 >= argc) {
                return false;
            }
            *interval_seconds = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return false;
        } else {
            return false;
        }
    }

    return *interval_seconds > 0;
}

void log_sample(bool foreground, std::string_view channel_id, double value,
                std::string_view unit) {
    syslog(LOG_INFO, "%.*s=%.2f %.*s", static_cast<int>(channel_id.size()),
           channel_id.data(), value, static_cast<int>(unit.size()),
           unit.data());
    if (foreground) {
        std::fprintf(stderr, "%.*s=%.2f %.*s\n",
                     static_cast<int>(channel_id.size()), channel_id.data(),
                     value, static_cast<int>(unit.size()), unit.data());
    }
}

} // namespace

int main(int argc, char *argv[]) {
    openlog(std::string(kToolName).c_str(), LOG_PID | LOG_CONS, LOG_DAEMON);

    bool foreground = false;
    std::string bus_path = kDefaultBus;
    std::uint8_t address = kDefaultAddress;
    int interval_seconds = 1;

    if (!parse_args(argc, argv, &foreground, &bus_path, &address,
                    &interval_seconds)) {
        print_usage();
        closelog();
        return EXIT_FAILURE;
    }

    std::unique_ptr<bossa::drivers::Driver> driver =
        bossa::drivers::Registry::create("bme280");
    if (!driver) {
        syslog(LOG_ERR, "bme280 driver not registered");
        closelog();
        return EXIT_FAILURE;
    }

    const nlohmann::json parameters = {
        {"bus", bus_path},
        {"address", address},
    };

    try {
        driver->configure(parameters);
    } catch (const std::exception &ex) {
        syslog(LOG_ERR, "configure failed: %s", ex.what());
        if (foreground) {
            std::fprintf(stderr, "configure failed: %s\n", ex.what());
        }
        closelog();
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "polling BME280 on %s address 0x%02x every %d s",
           bus_path.c_str(), address, interval_seconds);

    while (true) {
        const bossa::drivers::ReadResult result = driver->read();
        for (std::size_t index = 0; index < result.sample_count; ++index) {
            const bossa::telemetry::Sample &sample = result.samples[index];
            log_sample(foreground, sample.channel_id, sample.value,
                       sample.unit);
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
}
