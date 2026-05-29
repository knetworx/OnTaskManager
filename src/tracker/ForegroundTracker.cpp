#include "ForegroundTracker.h"

#include "providers/ChromeProvider.h"
#include "providers/GenericProvider.h"
#include "providers/VisualStudioProvider.h"

#include <windows.h>

namespace ontask::tracker {

namespace {

std::wstring baseName(const std::wstring& fullPath) {
    const auto pos = fullPath.find_last_of(L"\\/");
    return pos == std::wstring::npos ? fullPath : fullPath.substr(pos + 1);
}

} // namespace

ForegroundTracker::ForegroundTracker() {
    providers_.push_back(std::make_unique<ChromeProvider>());
    providers_.push_back(std::make_unique<VisualStudioProvider>());
    providers_.push_back(std::make_unique<GenericProvider>()); // fallback, registered last
}

ForegroundTracker::~ForegroundTracker() = default;

std::optional<ActivitySample> ForegroundTracker::sample() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return std::nullopt;

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, static_cast<int>(std::size(title)));

    std::wstring exeName;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
        wchar_t fullPath[MAX_PATH] = {};
        DWORD sz = static_cast<DWORD>(std::size(fullPath));
        if (QueryFullProcessImageNameW(h, 0, fullPath, &sz)) {
            exeName = baseName(fullPath);
        }
        CloseHandle(h);
    }

    if (exeName.empty()) return std::nullopt;

    ActivitySample s;
    s.timestamp = std::chrono::system_clock::now();
    for (const auto& provider : providers_) {
        if (provider->matches(exeName)) {
            s.path = provider->derivePath(exeName, title);
            if (!s.path.empty()) return s;
        }
    }
    return std::nullopt;
}

} // namespace ontask::tracker
