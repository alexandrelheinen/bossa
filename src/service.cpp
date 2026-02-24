/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <thread>
#include <unistd.h>

#include "bossa/service.hpp"

namespace bossa {

static volatile sig_atomic_t running = 1;

/**
 * @brief Signal handler to manage termination signals.
 *
 * @param sig The signal number.
 */
static void signalHandler(int sig) {
    switch (sig) {
    case SIGTERM:
    case SIGINT:
        running = 0;
        break;
    default:
        break;
    }
}

Service::Service(const std::string &name) : _name(name) {}

int Service::start() {
    // Initialize running flag
    running = 1;

    // Daemonize the process
    daemonize();

    // Setup signal handlers
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    syslog(LOG_INFO, "%s service started", _name.c_str());

    // Main service loop
    while (running) {
        this->loop();
    }

    syslog(LOG_INFO, "%s service stopped", _name.c_str());
    closelog();

    return EXIT_SUCCESS;
}

void Service::daemonize() {
    // Fork off the parent process
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Exit parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Change the file mode mask
    umask(0);

    // Create a new SID for the child process
    pid_t sid = setsid();
    if (sid < 0) {
        std::cerr << "Failed to create SID" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Change the current working directory
    if (chdir("/") < 0) {
        std::cerr << "Failed to change directory" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO) {
        dup2(fd, STDIN_FILENO);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    // Open syslog for logging
    openlog(_name.c_str(), LOG_PID | LOG_CONS, LOG_DAEMON);
}

} // namespace bossa
