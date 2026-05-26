#pragma once

#include <string>

namespace ontask::ui {

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    void setTooltip(const std::wstring& text);
};

} // namespace ontask::ui
