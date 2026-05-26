#include "TrayIcon.h"

namespace ontask::ui {

TrayIcon::TrayIcon() = default;
TrayIcon::~TrayIcon() = default;

void TrayIcon::setTooltip(const std::wstring& /*text*/) {
    // TODO: Shell_NotifyIconW with NIM_ADD / NIM_MODIFY once a host window exists.
}

} // namespace ontask::ui
