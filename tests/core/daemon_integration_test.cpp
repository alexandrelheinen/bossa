/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtest/gtest.h>

namespace {

std::filesystem::path write_temp_config(const std::string &contents) {
    static std::atomic<int> counter{0};
    const auto path =
        std::filesystem::temp_directory_path() /
        ("bossa_daemon_test_" + std::to_string(counter.fetch_add(1)) + ".yaml");

    std::ofstream output(path);
    output << contents;
    output.close();
    return path;
}

void remove_if_exists(const std::filesystem::path &path) {
    std::error_code error;
    std::filesystem::remove(path, error);
}

struct ProcessResult {
    int exit_code{-1};
    std::string stderr_output;
};

ProcessResult run_bossa_foreground(const std::string &binary_path,
                                   const std::filesystem::path &config_path) {
    int stderr_pipe[2]{-1, -1};
    if (pipe(stderr_pipe) != 0) {
        return ProcessResult{};
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return ProcessResult{};
    }

    if (pid == 0) {
        close(stderr_pipe[0]);
        if (dup2(stderr_pipe[1], STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(stderr_pipe[1]);

        const std::string config_arg = config_path.string();
        execl(binary_path.c_str(), binary_path.c_str(), "--foreground",
              "--config", config_arg.c_str(), static_cast<char *>(nullptr));
        _exit(127);
    }

    close(stderr_pipe[1]);

    std::string captured;
    std::array<char, 256> buffer{};
    ssize_t bytes_read = 0;
    while ((bytes_read = read(stderr_pipe[0], buffer.data(), buffer.size())) >
           0) {
        captured.append(buffer.data(), static_cast<std::size_t>(bytes_read));
    }
    close(stderr_pipe[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return ProcessResult{};
    }

    ProcessResult result;
    result.stderr_output = std::move(captured);
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    }
    return result;
}

} // namespace

#ifndef BOSSA_DAEMON_BINARY
#define BOSSA_DAEMON_BINARY "bossa"
#endif

// Phase 1 acceptance — invalid YAML exits 1 and surfaces config error output.
TEST(DaemonIntegrationTest, InvalidYamlExitsWithConfigError) {
    const auto path = write_temp_config("config_version: [not: valid: yaml");

    const auto result = run_bossa_foreground(BOSSA_DAEMON_BINARY, path);

    EXPECT_EQ(result.exit_code, EXIT_FAILURE);
    EXPECT_NE(result.stderr_output.find("config:"), std::string::npos);
    EXPECT_NE(result.stderr_output.find("YAML parse error"), std::string::npos);

    remove_if_exists(path);
}

// Phase 1 acceptance — missing config file exits 1 with a clear error.
TEST(DaemonIntegrationTest, MissingConfigExitsWithConfigError) {
    const auto path = std::filesystem::temp_directory_path() /
                      "bossa_missing_daemon_config.yaml";

    const auto result = run_bossa_foreground(BOSSA_DAEMON_BINARY, path);

    EXPECT_EQ(result.exit_code, EXIT_FAILURE);
    EXPECT_NE(result.stderr_output.find("config:"), std::string::npos);
    EXPECT_NE(result.stderr_output.find("not found"), std::string::npos);
}
