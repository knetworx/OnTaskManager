#include "Database.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace ontask::storage {

namespace {

constexpr const char* kSchema =
    "CREATE TABLE IF NOT EXISTS activity_samples ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  timestamp_unix_ms INTEGER NOT NULL,"
    "  process_name TEXT,"
    "  window_title TEXT,"
    "  idle_seconds INTEGER"
    ");";

}

Database::Database(const std::filesystem::path& path) {
    if (sqlite3_open(path.string().c_str(), &db_) != SQLITE_OK) {
        const std::string msg = db_ ? sqlite3_errmsg(db_) : "open failed";
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("failed to open database: " + msg);
    }

    char* err = nullptr;
    if (sqlite3_exec(db_, kSchema, nullptr, nullptr, &err) != SQLITE_OK) {
        const std::string msg = err ? err : "unknown";
        sqlite3_free(err);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("schema init failed: " + msg);
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

} // namespace ontask::storage
