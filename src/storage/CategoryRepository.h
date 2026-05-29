#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ontask::storage {

class Database;

struct CategoryNode {
    std::int64_t id = 0;
    std::optional<std::int64_t> parentId;
    std::wstring name;
    int position = 0;
};

class CategoryRepository {
public:
    explicit CategoryRepository(Database& db);

    std::int64_t createRoot(const std::wstring& name);
    std::int64_t createChild(std::int64_t parentId, const std::wstring& name);
    void rename(std::int64_t id, const std::wstring& newName);
    void remove(std::int64_t id);

    std::vector<CategoryNode> rootNodes();
    std::vector<CategoryNode> children(std::int64_t parentId);
    std::optional<CategoryNode> node(std::int64_t id);

private:
    Database& db_;
};

} // namespace ontask::storage
