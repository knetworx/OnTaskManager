#include "SampleRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace ontask::storage {

namespace {

std::int64_t toMs(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point fromMs(std::int64_t ms) {
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{ms}};
}

[[noreturn]] void throwSqlite(sqlite3* db, const char* what) {
    throw std::runtime_error(std::string(what) + ": " + sqlite3_errmsg(db));
}

} // namespace

SampleRepository::SampleRepository(Database& db) : db_(db) {}

void SampleRepository::insertActivity(std::int64_t activityNodeId,
                                      std::chrono::system_clock::time_point timestamp) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "INSERT INTO sample (timestamp_unix_ms, activity_node_id, is_idle) "
                           "VALUES (?, ?, 0);",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare insert activity sample");
    }
    sqlite3_bind_int64(stmt, 1, toMs(timestamp));
    sqlite3_bind_int64(stmt, 2, activityNodeId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "insert activity sample");
    }
    sqlite3_finalize(stmt);
}

void SampleRepository::insertIdle(std::chrono::system_clock::time_point timestamp) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "INSERT INTO sample (timestamp_unix_ms, activity_node_id, is_idle) "
                           "VALUES (?, NULL, 1);",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare insert idle sample");
    }
    sqlite3_bind_int64(stmt, 1, toMs(timestamp));
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "insert idle sample");
    }
    sqlite3_finalize(stmt);
}

std::vector<SampleRow> SampleRepository::samplesInRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT timestamp_unix_ms, activity_node_id, is_idle FROM sample "
                           "WHERE timestamp_unix_ms >= ? AND timestamp_unix_ms < ? "
                           "ORDER BY timestamp_unix_ms;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select samples in range");
    }
    sqlite3_bind_int64(stmt, 1, toMs(start));
    sqlite3_bind_int64(stmt, 2, toMs(end));
    std::vector<SampleRow> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SampleRow r;
        r.timestamp = fromMs(sqlite3_column_int64(stmt, 0));
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            r.activityNodeId = sqlite3_column_int64(stmt, 1);
        }
        r.isIdle = sqlite3_column_int(stmt, 2) != 0;
        out.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return out;
}

} // namespace ontask::storage
