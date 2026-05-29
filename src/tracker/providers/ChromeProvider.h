#pragma once

#include "ActivityProvider.h"

namespace ontask::tracker {

class ChromeProvider : public ActivityProvider {
public:
    bool matches(std::wstring_view exeName) const override;
    std::vector<std::wstring> derivePath(std::wstring_view exeName,
                                         std::wstring_view windowTitle) const override;
};

} // namespace ontask::tracker
