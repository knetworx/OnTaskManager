#include "ForegroundTracker.h"

#include <windows.h>

namespace ontask::tracker {

std::optional<ActivitySample> ForegroundTracker::sample() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        return std::nullopt;
    }

    ActivitySample s;
    s.timestamp = std::chrono::system_clock::now();

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, static_cast<int>(std::size(title)));
    s.windowTitle = title;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
        wchar_t path[MAX_PATH] = {};
        DWORD sz = static_cast<DWORD>(std::size(path));
        if (QueryFullProcessImageNameW(h, 0, path, &sz)) {
            std::wstring full(path);
            const auto pos = full.find_last_of(L"\\/");
            s.processName = (pos == std::wstring::npos) ? full : full.substr(pos + 1);
        }
        CloseHandle(h);
    }

    return s;
}

} // namespace ontask::tracker
