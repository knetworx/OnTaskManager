#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ontask::storage {

class Database;

struct ActivityNode {
    std::int64_t id = 0;
    std::optional<std::int64_t> parentId;
    std::wstring name;
};

class ActivityRepository {
public:
    explicit ActivityRepository(Database& db);

    // Walks (creating as needed) the path from the activity-tree roots down.
    // Returns the leaf node id. Empty path is rejected.
    std::int64_t resolvePath(const std::vector<std::wstring>& path);

    std::vector<ActivityNode> rootNodes();
    std::vector<ActivityNode> children(std::int64_t nodeId);
    std::optional<ActivityNode> node(std::int64_t nodeId);

    void assignToCategory(std::int64_t activityNodeId, std::int64_t categoryNodeId);
    void clearAssignment(std::int64_t activityNodeId);

    std::optional<std::int64_t> directMapping(std::int64_t activityNodeId);

    // Walks parent chain until a mapping is found. Returns 0 (Uncategorized) if
    // no ancestor is mapped.
    std::int64_t effectiveCategoryOrZero(std::int64_t activityNodeId);

private:
    std::int64_t getOrCreateChild(std::optional<std::int64_t> parentId,
                                  const std::wstring& name);

    Database& db_;
};

} // namespace ontask::storage
