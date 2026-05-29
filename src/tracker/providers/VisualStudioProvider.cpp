#include "VisualStudioProvider.h"

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

// Strip a parenthesised mode suffix like "file.cpp (Working Tree)" -> "file.cpp".
std::wstring stripParenMode(std::wstring_view s) {
    const auto open = s.find_last_of(L'(');
    if (open == std::wstring_view::npos) return std::wstring{s};
    if (s.back() != L')') return std::wstring{s};
    return trim(s.substr(0, open));
}

} // namespace

bool VisualStudioProvider::matches(std::wstring_view exeName) const {
    return iequals(exeName, L"devenv.exe");
}

std::vector<std::wstring> VisualStudioProvider::derivePath(std::wstring_view,
                                                           std::wstring_view windowTitle) const {
    // Typical title: "<file>[ (mode)] - <Project> - Microsoft Visual Studio".
    static constexpr std::wstring_view kVsSuffix = L" - Microsoft Visual Studio";
    std::wstring_view core = windowTitle;
    if (core.size() >= kVsSuffix.size() &&
        core.substr(core.size() - kVsSuffix.size()) == kVsSuffix) {
        core = core.substr(0, core.size() - kVsSuffix.size());
    }

    if (core.empty()) {
        return {L"Visual Studio"};
    }

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

    std::vector<std::wstring> path{L"Visual Studio"};
    if (segments.size() >= 2) {
        // Last segment is project, first is file (possibly with mode).
        path.push_back(std::move(segments.back()));
        path.push_back(stripParenMode(segments.front()));
    } else if (!segments.empty()) {
        path.push_back(stripParenMode(segments.front()));
    }
    return path;
}

} // namespace ontask::tracker
