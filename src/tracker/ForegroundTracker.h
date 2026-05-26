#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace ontask::tracker {

struct ActivitySample {
    std::wstring processName;
    std::wstring windowTitle;
    std::chrono::system_clock::time_point timestamp;
};

class ForegroundTracker {
public:
    std::optional<ActivitySample> sample();
};

} // namespace ontask::tracker
