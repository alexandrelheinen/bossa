/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <syslog.h>
#include <thread>

#include "bossa/service.hpp"

/**
 * @brief Sample service that prints "Hello world" every 5 seconds.
 */
class SampleService : public bossa::Service {
  public:
    SampleService() : bossa::Service("bossa") {}

    void loop() override {
        // Print "Hello world" every 5 seconds
        syslog(LOG_INFO, "Hello world");
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
};
int main() {
    SampleService service;
    return service.start();
}
