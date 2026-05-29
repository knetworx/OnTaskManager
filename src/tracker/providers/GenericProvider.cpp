#include "GenericProvider.h"

#include <cwctype>

namespace ontask::tracker {

namespace {

std::wstring stripExtension(std::wstring_view exeName) {
    const auto dot = exeName.find_last_of(L'.');
    std::wstring base{dot == std::wstring_view::npos ? exeName : exeName.substr(0, dot)};
    if (!base.empty()) {
        base[0] = static_cast<wchar_t>(std::towupper(base[0]));
    }
    return base;
}

} // namespace

bool GenericProvider::matches(std::wstring_view) const {
    return true;
}

std::vector<std::wstring> GenericProvider::derivePath(std::wstring_view exeName,
                                                      std::wstring_view) const {
    std::wstring base = stripExtension(exeName);
    if (base.empty()) {
        base = L"Unknown";
    }
    return {base};
}

} // namespace ontask::tracker
