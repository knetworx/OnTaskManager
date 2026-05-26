#pragma once

#include <filesystem>

struct sqlite3;

namespace ontask::storage {

class Database {
public:
    explicit Database(const std::filesystem::path& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

} // namespace ontask::storage
