#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <thread>
#include <unistd.h>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
  if (sig == SIGTERM || sig == SIGINT) {
    running = 0;
  }
}

void daemonize() {
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
  openlog("bossa", LOG_PID | LOG_CONS, LOG_DAEMON);
}

int main() {
  // Daemonize the process
  daemonize();

  // Setup signal handlers
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  syslog(LOG_INFO, "bossa service started");

  // Main service loop
  while (running) {
    syslog(LOG_INFO, "hello world");
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  syslog(LOG_INFO, "bossa service stopped");
  closelog();

  return EXIT_SUCCESS;
}
