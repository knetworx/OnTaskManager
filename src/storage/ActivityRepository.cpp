#include "ActivityRepository.h"

#include "Database.h"

#include <sqlite3.h>
#include <windows.h>

#include <stdexcept>

namespace ontask::storage {

namespace {

std::string toUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                        out.data(), needed, nullptr, nullptr);
    return out;
}

std::wstring fromUtf8(const char* s) {
    if (!s || !*s) return {};
    const int needed = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (needed <= 1) return {};
    std::wstring out(static_cast<std::size_t>(needed - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), needed);
    return out;
}

[[noreturn]] void throwSqlite(sqlite3* db, const char* what) {
    throw std::runtime_error(std::string(what) + ": " + sqlite3_errmsg(db));
}

} // namespace

ActivityRepository::ActivityRepository(Database& db) : db_(db) {}

std::int64_t ActivityRepository::getOrCreateChild(std::optional<std::int64_t> parentId,
                                                  const std::wstring& name) {
    const std::string nameUtf8 = toUtf8(name);

    sqlite3_stmt* sel = nullptr;
    const char* selSql = parentId.has_value()
        ? "SELECT id FROM activity_node WHERE parent_id = ? AND name = ?;"
        : "SELECT id FROM activity_node WHERE parent_id IS NULL AND name = ?;";
    if (sqlite3_prepare_v2(db_.handle(), selSql, -1, &sel, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select activity_node");
    }
    if (parentId.has_value()) {
        sqlite3_bind_int64(sel, 1, *parentId);
        sqlite3_bind_text(sel, 2, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_text(sel, 1, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (sqlite3_step(sel) == SQLITE_ROW) {
        const std::int64_t id = sqlite3_column_int64(sel, 0);
        sqlite3_finalize(sel);
        return id;
    }
    sqlite3_finalize(sel);

    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "INSERT INTO activity_node (parent_id, name) VALUES (?, ?);",
                           -1, &ins, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare insert activity_node");
    }
    if (parentId.has_value()) {
        sqlite3_bind_int64(ins, 1, *parentId);
    } else {
        sqlite3_bind_null(ins, 1);
    }
    sqlite3_bind_text(ins, 2, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(ins) != SQLITE_DONE) {
        sqlite3_finalize(ins);
        throwSqlite(db_.handle(), "insert activity_node");
    }
    sqlite3_finalize(ins);
    return sqlite3_last_insert_rowid(db_.handle());
}

std::int64_t ActivityRepository::resolvePath(const std::vector<std::wstring>& path) {
    if (path.empty()) {
        throw std::invalid_argument("resolvePath: empty path");
    }
    std::optional<std::int64_t> current;
    for (const auto& segment : path) {
        current = getOrCreateChild(current, segment);
    }
    return *current;
}

std::vector<ActivityNode> ActivityRepository::rootNodes() {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT id, parent_id, name FROM activity_node "
                           "WHERE parent_id IS NULL ORDER BY name;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select root activity_nodes");
    }
    std::vector<ActivityNode> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ActivityNode n;
        n.id = sqlite3_column_int64(stmt, 0);
        n.parentId = std::nullopt;
        n.name = fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        out.push_back(std::move(n));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<ActivityNode> ActivityRepository::children(std::int64_t nodeId) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT id, parent_id, name FROM activity_node "
                           "WHERE parent_id = ? ORDER BY name;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select children activity_nodes");
    }
    sqlite3_bind_int64(stmt, 1, nodeId);
    std::vector<ActivityNode> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ActivityNode n;
        n.id = sqlite3_column_int64(stmt, 0);
        n.parentId = sqlite3_column_int64(stmt, 1);
        n.name = fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        out.push_back(std::move(n));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::optional<ActivityNode> ActivityRepository::node(std::int64_t nodeId) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT id, parent_id, name FROM activity_node WHERE id = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select activity_node by id");
    }
    sqlite3_bind_int64(stmt, 1, nodeId);
    std::optional<ActivityNode> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ActivityNode n;
        n.id = sqlite3_column_int64(stmt, 0);
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            n.parentId = sqlite3_column_int64(stmt, 1);
        }
        n.name = fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        out = std::move(n);
    }
    sqlite3_finalize(stmt);
    return out;
}

void ActivityRepository::assignToCategory(std::int64_t activityNodeId,
                                          std::int64_t categoryNodeId) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "INSERT INTO activity_category_mapping "
                           "  (activity_node_id, category_node_id) VALUES (?, ?) "
                           "ON CONFLICT(activity_node_id) DO UPDATE SET "
                           "  category_node_id = excluded.category_node_id;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare upsert mapping");
    }
    sqlite3_bind_int64(stmt, 1, activityNodeId);
    sqlite3_bind_int64(stmt, 2, categoryNodeId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "upsert mapping");
    }
    sqlite3_finalize(stmt);
}

void ActivityRepository::clearAssignment(std::int64_t activityNodeId) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "DELETE FROM activity_category_mapping WHERE activity_node_id = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare delete mapping");
    }
    sqlite3_bind_int64(stmt, 1, activityNodeId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "delete mapping");
    }
    sqlite3_finalize(stmt);
}

std::optional<std::int64_t> ActivityRepository::directMapping(std::int64_t activityNodeId) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT category_node_id FROM activity_category_mapping "
                           "WHERE activity_node_id = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select mapping");
    }
    sqlite3_bind_int64(stmt, 1, activityNodeId);
    std::optional<std::int64_t> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::int64_t ActivityRepository::effectiveCategoryOrZero(std::int64_t activityNodeId) {
    std::optional<std::int64_t> cur = activityNodeId;
    while (cur.has_value()) {
        if (auto direct = directMapping(*cur)) {
            return *direct;
        }
        auto n = node(*cur);
        if (!n) return 0;
        cur = n->parentId;
    }
    return 0;
}

} // namespace ontask::storage
