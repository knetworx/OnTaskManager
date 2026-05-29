#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace ontask::storage {

class Database;

struct SampleRow {
    std::chrono::system_clock::time_point timestamp;
    std::optional<std::int64_t> activityNodeId;
    bool isIdle = false;
};

class SampleRepository {
public:
    explicit SampleRepository(Database& db);

    void insertActivity(std::int64_t activityNodeId,
                        std::chrono::system_clock::time_point timestamp);
    void insertIdle(std::chrono::system_clock::time_point timestamp);

    // Half-open interval [start, end).
    std::vector<SampleRow> samplesInRange(std::chrono::system_clock::time_point start,
                                          std::chrono::system_clock::time_point end);

private:
    Database& db_;
};

} // namespace ontask::storage
