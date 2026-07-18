/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "bossa/telemetry/stored_sample.hpp"

struct sqlite3;

namespace bossa::storage {

/** @brief Row returned from the offline upload queue. */
struct PendingRow {
    std::int64_t id{0};
    telemetry::StoredSample sample;
};

/**
 * @brief SQLite-backed offline queue for failed uploads.
 *
 * Uses WAL mode. Not used on the hot sampling path.
 */
class LocalStore {
  public:
    LocalStore();
    ~LocalStore();

    LocalStore(const LocalStore &) = delete;
    LocalStore &operator=(const LocalStore &) = delete;

    /**
     * @brief Open or create the database and apply schema.
     * @param path Filesystem path to the SQLite file.
     * @return true on success.
     */
    bool open(const std::filesystem::path &path);

    /** @brief Close the database handle. */
    void close();

    /**
     * @brief Insert a sample into pending_uploads.
     * @param sample Sample to persist.
     * @return true on success.
     */
    bool enqueue(const telemetry::StoredSample &sample);

    /**
     * @brief Peek the oldest pending rows without deleting them.
     * @param max_count Maximum rows to return.
     * @return Rows in insertion order.
     */
    std::vector<PendingRow> peek_batch(std::size_t max_count) const;

    /**
     * @brief Delete a pending row after a successful upload.
     * @param id Row id from peek_batch.
     * @return true on success.
     */
    bool acknowledge(std::int64_t id);

    /** @brief Number of pending rows. */
    std::size_t pending_count() const;

    /** @brief Last error message from SQLite, if any. */
    const std::string &last_error() const;

  private:
    bool exec(const char *sql);
    bool ensure_schema();

    ::sqlite3 *db_{nullptr};
    std::string last_error_;
};

} // namespace bossa::storage
