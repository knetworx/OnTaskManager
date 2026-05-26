#include "IdleDetector.h"

#include <windows.h>

namespace ontask::tracker {

std::chrono::seconds IdleDetector::idleTime() const {
    LASTINPUTINFO lii{};
    lii.cbSize = sizeof(lii);
    if (!GetLastInputInfo(&lii)) {
        return std::chrono::seconds::zero();
    }
    const DWORD now = GetTickCount();
    const DWORD deltaMs = (now >= lii.dwTime) ? (now - lii.dwTime) : 0;
    return std::chrono::seconds(deltaMs / 1000);
}

} // namespace ontask::tracker
