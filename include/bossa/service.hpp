/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   All rights reserved.
 *   Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <string>

namespace bossa {

/**
 * @brief Base class for creating a daemon service.
 *
 * @details This class provides the necessary functionality to daemonize a
 * process, handle signals, and run a main service loop. Derived classes should
 * implement the `loop` method to define the service's behavior.
 */
class Service {
  public:
    /**
     * @brief Construct a new Service object.
     *
     * @param name The name of the service.
     */
    Service(const std::string &name);

    /**
     * @brief Destroy the Service object.
     */
    virtual ~Service() = default;

    /**
     * @brief Start the service.
     */
    int start();

    /**
     * @brief Service main loop function.
     *
     * @details This function is run continuously to handle service tasks.
     */
    virtual void loop() = 0;

  private:
    // Name of the service, for logging purposes
    const std::string _name;

    /**
     * @brief Daemonize the process.
     */
    void daemonize();
};
} // namespace bossa