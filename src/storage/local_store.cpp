/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/storage/local_store.hpp"

#include <cstdio>
#include <system_error>

#include <sqlite3.h>

namespace bossa::storage {

namespace {

std::int64_t time_to_unix_ms(std::chrono::system_clock::time_point timestamp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               timestamp.time_since_epoch())
        .count();
}

std::chrono::system_clock::time_point unix_ms_to_time(std::int64_t millis) {
    return std::chrono::system_clock::time_point{
        std::chrono::milliseconds{millis}};
}

const char *quality_to_string(telemetry::SampleQuality quality) {
    switch (quality) {
    case telemetry::SampleQuality::kGood:
        return "good";
    case telemetry::SampleQuality::kUncertain:
        return "uncertain";
    case telemetry::SampleQuality::kBad:
        return "bad";
    }
    return "good";
}

telemetry::SampleQuality quality_from_string(const char *text) {
    if (text == nullptr) {
        return telemetry::SampleQuality::kGood;
    }
    const std::string value(text);
    if (value == "uncertain") {
        return telemetry::SampleQuality::kUncertain;
    }
    if (value == "bad") {
        return telemetry::SampleQuality::kBad;
    }
    return telemetry::SampleQuality::kGood;
}

} // namespace

LocalStore::LocalStore() = default;

LocalStore::~LocalStore() { close(); }

bool LocalStore::open(const std::filesystem::path &path) {
    close();
    if (path.empty()) {
        last_error_ = "local store path is empty";
        return false;
    }

    if (path.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            last_error_ = "cannot create directory: " + error.message();
            return false;
        }
    }

    if (sqlite3_open(path.string().c_str(), &db_) != SQLITE_OK) {
        last_error_ = db_ ? sqlite3_errmsg(db_) : "sqlite3_open failed";
        close();
        return false;
    }

    if (!exec("PRAGMA journal_mode=WAL;") ||
        !exec("PRAGMA synchronous=NORMAL;")) {
        close();
        return false;
    }

    if (!ensure_schema()) {
        close();
        return false;
    }

    last_error_.clear();
    return true;
}

void LocalStore::close() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool LocalStore::exec(const char *sql) {
    char *error_message = nullptr;
    const int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK) {
        last_error_ = error_message ? error_message : "sqlite exec failed";
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

bool LocalStore::ensure_schema() {
    return exec("CREATE TABLE IF NOT EXISTS pending_uploads ("
                "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "  channel_id TEXT NOT NULL,"
                "  value REAL NOT NULL,"
                "  unit TEXT NOT NULL,"
                "  timestamp_ms INTEGER NOT NULL,"
                "  quality TEXT NOT NULL,"
                "  priority INTEGER NOT NULL,"
                "  created_at_ms INTEGER NOT NULL"
                ");");
}

bool LocalStore::enqueue(const telemetry::StoredSample &sample) {
    if (db_ == nullptr) {
        last_error_ = "local store is not open";
        return false;
    }

    constexpr const char *kSql =
        "INSERT INTO pending_uploads "
        "(channel_id, value, unit, timestamp_ms, quality, priority, "
        "created_at_ms) VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &statement, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }

    const auto now_ms = time_to_unix_ms(std::chrono::system_clock::now());
    sqlite3_bind_text(statement, 1, sample.channel_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(statement, 2, sample.value);
    sqlite3_bind_text(statement, 3, sample.unit, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 4, time_to_unix_ms(sample.timestamp));
    sqlite3_bind_text(statement, 5, quality_to_string(sample.quality), -1,
                      SQLITE_STATIC);
    sqlite3_bind_int(statement, 6, static_cast<int>(sample.priority));
    sqlite3_bind_int64(statement, 7, now_ms);

    const int rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    last_error_.clear();
    return true;
}

std::vector<PendingRow> LocalStore::peek_batch(std::size_t max_count) const {
    std::vector<PendingRow> rows;
    if (db_ == nullptr || max_count == 0) {
        return rows;
    }

    constexpr const char *kSql =
        "SELECT id, channel_id, value, unit, timestamp_ms, quality, priority "
        "FROM pending_uploads ORDER BY id ASC LIMIT ?;";

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &statement, nullptr) != SQLITE_OK) {
        return rows;
    }

    sqlite3_bind_int64(statement, 1, static_cast<sqlite3_int64>(max_count));
    while (sqlite3_step(statement) == SQLITE_ROW) {
        PendingRow row;
        row.id = sqlite3_column_int64(statement, 0);
        const auto *channel_id =
            reinterpret_cast<const char *>(sqlite3_column_text(statement, 1));
        row.sample.value = sqlite3_column_double(statement, 2);
        const auto *unit_text =
            reinterpret_cast<const char *>(sqlite3_column_text(statement, 3));
        row.sample.timestamp =
            unix_ms_to_time(sqlite3_column_int64(statement, 4));
        const auto *quality =
            reinterpret_cast<const char *>(sqlite3_column_text(statement, 5));
        row.sample.quality = quality_from_string(quality);
        row.sample.priority =
            static_cast<telemetry::Priority>(sqlite3_column_int(statement, 6));

        if (channel_id != nullptr) {
            std::snprintf(row.sample.channel_id, sizeof(row.sample.channel_id),
                          "%s", channel_id);
        }
        if (unit_text != nullptr) {
            std::snprintf(row.sample.unit, sizeof(row.sample.unit), "%s",
                          unit_text);
        }
        rows.push_back(row);
    }
    sqlite3_finalize(statement);
    return rows;
}

bool LocalStore::acknowledge(std::int64_t id) {
    if (db_ == nullptr) {
        last_error_ = "local store is not open";
        return false;
    }

    constexpr const char *kSql = "DELETE FROM pending_uploads WHERE id = ?;";
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &statement, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int64(statement, 1, id);
    const int rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

std::size_t LocalStore::pending_count() const {
    if (db_ == nullptr) {
        return 0;
    }
    constexpr const char *kSql = "SELECT COUNT(*) FROM pending_uploads;";
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &statement, nullptr) != SQLITE_OK) {
        return 0;
    }
    std::size_t count = 0;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        count = static_cast<std::size_t>(sqlite3_column_int64(statement, 0));
    }
    sqlite3_finalize(statement);
    return count;
}

const std::string &LocalStore::last_error() const { return last_error_; }

} // namespace bossa::storage
