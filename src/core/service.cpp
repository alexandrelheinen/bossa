/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/core/service.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#if defined(HAVE_SYSTEMD)
#include <systemd/sd-daemon.h>
#endif

namespace bossa::core {

namespace {

volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_reload = 0;

void signal_handler(int sig) {
    switch (sig) {
    case SIGTERM:
    case SIGINT:
        g_running = 0;
        break;
    case SIGHUP:
        g_reload = 1;
        break;
    default:
        break;
    }
}

} // namespace

Service::Service(std::string name, ServiceOptions options)
    : name_(std::move(name)), options_(options) {}

void Service::request_stop() { g_running = 0; }

void Service::request_reload() { g_reload = 1; }

bool Service::is_running() { return g_running != 0; }

bool Service::is_reload_requested() { return g_reload != 0; }

void Service::reload() {
    syslog(LOG_INFO, "%s: reload requested (no-op base implementation)",
           name_.c_str());
}

void Service::consume_reload_if_needed() {
    if (g_reload == 0) {
        return;
    }
    g_reload = 0;
    reload();
}

int Service::start() {
    g_running = 1;
    g_reload = 0;

    if (!options_.foreground) {
        daemonize();
    } else {
        open_logging();
    }

    if (!install_signal_handlers()) {
        syslog(LOG_ERR, "%s: failed to install signal handlers", name_.c_str());
        closelog();
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "%s service started", name_.c_str());

#if defined(HAVE_SYSTEMD)
    if (sd_notify(0, "READY=1") < 0) {
        syslog(LOG_WARNING, "%s: sd_notify READY=1 failed", name_.c_str());
    }
#endif

    while (g_running) {
        consume_reload_if_needed();
        loop();
    }

    syslog(LOG_INFO, "%s service stopped", name_.c_str());
    closelog();
    return EXIT_SUCCESS;
}

void Service::open_logging() {
    openlog(name_.c_str(), LOG_PID | LOG_CONS, LOG_DAEMON);
}

bool Service::install_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGTERM, &action, nullptr) < 0) {
        return false;
    }
    if (sigaction(SIGINT, &action, nullptr) < 0) {
        return false;
    }
    if (sigaction(SIGHUP, &action, nullptr) < 0) {
        return false;
    }
    return true;
}

void Service::daemonize() {
    const pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "%s: fork failed\n", name_.c_str());
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    const pid_t sid = setsid();
    if (sid < 0) {
        fprintf(stderr, "%s: setsid failed\n", name_.c_str());
        exit(EXIT_FAILURE);
    }

    if (chdir("/") < 0) {
        fprintf(stderr, "%s: chdir failed\n", name_.c_str());
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    const int null_fd = open("/dev/null", O_RDWR);
    if (null_fd < 0) {
        exit(EXIT_FAILURE);
    }

    if (dup2(null_fd, STDIN_FILENO) < 0 || dup2(null_fd, STDOUT_FILENO) < 0 ||
        dup2(null_fd, STDERR_FILENO) < 0) {
        close(null_fd);
        exit(EXIT_FAILURE);
    }

    if (null_fd > STDERR_FILENO) {
        close(null_fd);
    }

    open_logging();
}

} // namespace bossa::core
