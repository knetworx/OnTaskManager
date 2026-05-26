#pragma once

#include "ForegroundTracker.h"

#include <chrono>

namespace ontask::storage {

class Database;

class ActivityRepository {
public:
    explicit ActivityRepository(Database& db);

    void insert(const tracker::ActivitySample& sample, std::chrono::seconds idleSeconds);

private:
    Database& db_;
};

} // namespace ontask::storage
