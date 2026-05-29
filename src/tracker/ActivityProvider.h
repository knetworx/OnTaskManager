#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ontask::tracker {

class ActivityProvider {
public:
    virtual ~ActivityProvider() = default;

    virtual bool matches(std::wstring_view exeName) const = 0;
    virtual std::vector<std::wstring> derivePath(std::wstring_view exeName,
                                                 std::wstring_view windowTitle) const = 0;
};

} // namespace ontask::tracker
