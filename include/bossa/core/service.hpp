/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace bossa::core {

/**
 * @brief Options that control daemon startup behavior.
 */
struct ServiceOptions {
    /** @brief When true, skip fork/setsid and keep stdio attached. */
    bool foreground = false;
};

/**
 * @brief Base class for creating a daemon service.
 *
 * @details Provides daemonization, signal handling, and a main service loop.
 * Derived classes implement loop() to define service behavior. SIGHUP sets a
 * reload flag; subclasses override reload() to apply new configuration.
 */
class Service {
  public:
    /**
     * @brief Construct a Service.
     * @param name Service name used for syslog identification.
     * @param options Startup options (foreground mode, etc.).
     */
    Service(std::string name, ServiceOptions options = {});

    virtual ~Service() = default;

    /**
     * @brief Start the service and run until a stop signal is received.
     * @return EXIT_SUCCESS on clean shutdown, EXIT_FAILURE on setup error.
     */
    int start();

    /**
     * @brief Service main loop — called repeatedly until shutdown.
     */
    virtual void loop() = 0;

    /**
     * @brief Apply a configuration reload requested by SIGHUP.
     *
     * Default implementation is a no-op. Override to re-read config.
     */
    virtual void reload();

    /**
     * @brief Request graceful shutdown (test and foreground use).
     */
    static void request_stop();

    /**
     * @brief Request configuration reload (test and SIGHUP use).
     */
    static void request_reload();

    /**
     * @brief Check whether the service loop should continue.
     * @return true while the service is running.
     */
    static bool is_running();

    /**
     * @brief True when a reload was requested and not yet consumed.
     * @return Reload pending flag.
     */
    static bool is_reload_requested();

  private:
    void daemonize();
    bool install_signal_handlers();
    void open_logging();
    void consume_reload_if_needed();

    std::string name_;
    ServiceOptions options_;
};

} // namespace bossa::core
