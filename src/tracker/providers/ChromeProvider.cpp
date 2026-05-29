#include "ChromeProvider.h"

#include <algorithm>
#include <cwctype>

namespace ontask::tracker {

namespace {

bool iequals(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::towlower(a[i]) != std::towlower(b[i])) return false;
    }
    return true;
}

std::wstring trim(std::wstring_view s) {
    std::size_t lo = 0, hi = s.size();
    while (lo < hi && std::iswspace(s[lo])) ++lo;
    while (hi > lo && std::iswspace(s[hi - 1])) --hi;
    return std::wstring{s.substr(lo, hi - lo)};
}

// Chrome window titles end with " - Google Chrome" (Edge: " - Microsoft​ Edge" or
// " and N more pages - Personal - Microsoft​ Edge"). Strip that tail.
std::wstring_view stripBrowserSuffix(std::wstring_view title) {
    static constexpr std::wstring_view kSuffixes[] = {
        L" - Google Chrome",
        L" - Microsoft​ Edge",
        L" - Microsoft Edge",
    };
    for (const auto suf : kSuffixes) {
        if (title.size() >= suf.size() &&
            iequals(title.substr(title.size() - suf.size()), suf)) {
            return title.substr(0, title.size() - suf.size());
        }
    }
    return title;
}

} // namespace

bool ChromeProvider::matches(std::wstring_view exeName) const {
    return iequals(exeName, L"chrome.exe") || iequals(exeName, L"msedge.exe");
}

std::vector<std::wstring> ChromeProvider::derivePath(std::wstring_view exeName,
                                                     std::wstring_view windowTitle) const {
    const std::wstring app = iequals(exeName, L"msedge.exe") ? L"Edge" : L"Chrome";

    const std::wstring_view core = stripBrowserSuffix(windowTitle);
    if (core.empty()) {
        return {app};
    }

    // Split on " - " (with surrounding spaces); typical pattern: "<page> - <site>".
    static constexpr std::wstring_view kSep = L" - ";
    std::vector<std::wstring> segments;
    std::size_t start = 0;
    while (start <= core.size()) {
        const std::size_t pos = core.find(kSep, start);
        if (pos == std::wstring_view::npos) {
            segments.push_back(trim(core.substr(start)));
            break;
        }
        segments.push_back(trim(core.substr(start, pos - start)));
        start = pos + kSep.size();
    }

    // Heuristic: typical Chrome title is "<page> - <site>". The last segment is
    // the site, earlier segments are page-context (possibly multiple).
    if (segments.size() >= 2) {
        std::wstring site = segments.back();
        segments.pop_back();

        std::vector<std::wstring> path;
        path.push_back(app);
        path.push_back(std::move(site));

        // For Reddit, the page often looks like "Title : r/Subreddit"; lift the
        // subreddit out as its own level so the activity tree shows
        // Chrome / Reddit / r/Subreddit / <title>.
        if (iequals(path.back(), L"Reddit") && !segments.empty()) {
            const std::wstring& page = segments.front();
            const auto colon = page.find(L" : r/");
            if (colon != std::wstring::npos) {
                std::wstring sub = trim(page.substr(colon + 3));
                std::wstring title = trim(page.substr(0, colon));
                path.push_back(std::move(sub));
                if (!title.empty()) path.push_back(std::move(title));
                return path;
            }
        }

        for (auto& seg : segments) {
            if (!seg.empty()) path.push_back(std::move(seg));
        }
        return path;
    }

    // Single segment: just the page title, with no separator.
    return {app, segments.empty() ? std::wstring{core} : std::move(segments.front())};
}

} // namespace ontask::tracker
