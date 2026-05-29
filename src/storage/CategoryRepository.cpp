#include "CategoryRepository.h"

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

CategoryRepository::CategoryRepository(Database& db) : db_(db) {}

std::int64_t CategoryRepository::createRoot(const std::wstring& name) {
    const std::string nameUtf8 = toUtf8(name);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "INSERT INTO category_node (parent_id, name) VALUES (NULL, ?);",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare insert category root");
    }
    sqlite3_bind_text(stmt, 1, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "insert category root");
    }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db_.handle());
}

std::int64_t CategoryRepository::createChild(std::int64_t parentId, const std::wstring& name) {
    const std::string nameUtf8 = toUtf8(name);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "INSERT INTO category_node (parent_id, name) VALUES (?, ?);",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare insert category child");
    }
    sqlite3_bind_int64(stmt, 1, parentId);
    sqlite3_bind_text(stmt, 2, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "insert category child");
    }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db_.handle());
}

void CategoryRepository::rename(std::int64_t id, const std::wstring& newName) {
    const std::string nameUtf8 = toUtf8(newName);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "UPDATE category_node SET name = ? WHERE id = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare rename category");
    }
    sqlite3_bind_text(stmt, 1, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "rename category");
    }
    sqlite3_finalize(stmt);
}

void CategoryRepository::remove(std::int64_t id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), "DELETE FROM category_node WHERE id = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare delete category");
    }
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwSqlite(db_.handle(), "delete category");
    }
    sqlite3_finalize(stmt);
}

std::vector<CategoryNode> CategoryRepository::rootNodes() {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT id, parent_id, name, position FROM category_node "
                           "WHERE parent_id IS NULL ORDER BY position, name;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select root categories");
    }
    std::vector<CategoryNode> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CategoryNode n;
        n.id = sqlite3_column_int64(stmt, 0);
        n.name = fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        n.position = sqlite3_column_int(stmt, 3);
        out.push_back(std::move(n));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<CategoryNode> CategoryRepository::children(std::int64_t parentId) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT id, parent_id, name, position FROM category_node "
                           "WHERE parent_id = ? ORDER BY position, name;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select category children");
    }
    sqlite3_bind_int64(stmt, 1, parentId);
    std::vector<CategoryNode> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CategoryNode n;
        n.id = sqlite3_column_int64(stmt, 0);
        n.parentId = sqlite3_column_int64(stmt, 1);
        n.name = fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        n.position = sqlite3_column_int(stmt, 3);
        out.push_back(std::move(n));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::optional<CategoryNode> CategoryRepository::node(std::int64_t id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(),
                           "SELECT id, parent_id, name, position FROM category_node WHERE id = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        throwSqlite(db_.handle(), "prepare select category by id");
    }
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<CategoryNode> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CategoryNode n;
        n.id = sqlite3_column_int64(stmt, 0);
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            n.parentId = sqlite3_column_int64(stmt, 1);
        }
        n.name = fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        n.position = sqlite3_column_int(stmt, 3);
        out = std::move(n);
    }
    sqlite3_finalize(stmt);
    return out;
}

} // namespace ontask::storage
