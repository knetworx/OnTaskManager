#pragma once

#include "ActivityProvider.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ontask::tracker {

struct ActivitySample {
    std::vector<std::wstring> path;
    std::chrono::system_clock::time_point timestamp;
};

class ForegroundTracker {
public:
    ForegroundTracker();
    ~ForegroundTracker();

    ForegroundTracker(const ForegroundTracker&) = delete;
    ForegroundTracker& operator=(const ForegroundTracker&) = delete;

    std::optional<ActivitySample> sample();

private:
    std::vector<std::unique_ptr<ActivityProvider>> providers_;
};

} // namespace ontask::tracker
