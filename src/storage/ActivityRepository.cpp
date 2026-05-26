#include "ActivityRepository.h"

#include "Database.h"

#include <sqlite3.h>
#include <windows.h>

#include <stdexcept>
#include <string>

namespace ontask::storage {

namespace {

// TODO: replace with a proper UTF-16 -> UTF-8 helper once we add a strings module.
std::string toUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                        out.data(), needed, nullptr, nullptr);
    return out;
}

} // namespace

ActivityRepository::ActivityRepository(Database& db) : db_(db) {}

void ActivityRepository::insert(const tracker::ActivitySample& sample,
                                std::chrono::seconds idleSeconds) {
    static constexpr const char* sql =
        "INSERT INTO activity_samples (timestamp_unix_ms, process_name, window_title, idle_seconds)"
        " VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db_.handle()));
    }

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        sample.timestamp.time_since_epoch())
                        .count();
    const std::string processUtf8 = toUtf8(sample.processName);
    const std::string titleUtf8 = toUtf8(sample.windowTitle);

    sqlite3_bind_int64(stmt, 1, ms);
    sqlite3_bind_text(stmt, 2, processUtf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, titleUtf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, idleSeconds.count());

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db_.handle()));
    }
}

} // namespace ontask::storage
