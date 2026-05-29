#include "Database.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace ontask::storage {

namespace {

constexpr const char* kSchema =
    "PRAGMA foreign_keys = ON;"
    "CREATE TABLE IF NOT EXISTS category_node ("
    "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  parent_id INTEGER REFERENCES category_node(id) ON DELETE CASCADE,"
    "  name      TEXT NOT NULL,"
    "  position  INTEGER NOT NULL DEFAULT 0,"
    "  UNIQUE(parent_id, name)"
    ");"
    "CREATE TABLE IF NOT EXISTS activity_node ("
    "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  parent_id INTEGER REFERENCES activity_node(id) ON DELETE CASCADE,"
    "  name      TEXT NOT NULL,"
    "  UNIQUE(parent_id, name)"
    ");"
    "CREATE TABLE IF NOT EXISTS activity_category_mapping ("
    "  activity_node_id INTEGER PRIMARY KEY"
    "      REFERENCES activity_node(id) ON DELETE CASCADE,"
    "  category_node_id INTEGER NOT NULL"
    "      REFERENCES category_node(id) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS sample ("
    "  id                INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  timestamp_unix_ms INTEGER NOT NULL,"
    "  activity_node_id  INTEGER REFERENCES activity_node(id) ON DELETE SET NULL,"
    "  is_idle           INTEGER NOT NULL DEFAULT 0"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_sample_time  ON sample(timestamp_unix_ms);"
    "CREATE INDEX IF NOT EXISTS idx_activity_par ON activity_node(parent_id);"
    "CREATE INDEX IF NOT EXISTS idx_category_par ON category_node(parent_id);"
    "CREATE INDEX IF NOT EXISTS idx_mapping_cat  ON activity_category_mapping(category_node_id);";

} // namespace

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
