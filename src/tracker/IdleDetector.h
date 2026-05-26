#pragma once

#include <chrono>

namespace ontask::tracker {

class IdleDetector {
public:
    std::chrono::seconds idleTime() const;
};

} // namespace ontask::tracker
