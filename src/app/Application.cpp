#include "Application.h"

#include <iostream>

namespace ontask {

Application::Application(const std::filesystem::path& dataDir)
    : db_(dataDir / "ontask.db"),
      repo_(db_) {}

int Application::run() {
    tray_.setTooltip(L"OnTaskManager");
    window_.hide();

    if (auto sample = foregroundTracker_.sample()) {
        const auto idle = idleDetector_.idleTime();
        repo_.insert(*sample, idle);
        std::wcout << L"captured: " << sample->processName
                   << L" | " << sample->windowTitle
                   << L" | idle=" << idle.count() << L"s\n";
    } else {
        std::cout << "no foreground window detected\n";
    }

    return 0;
}

} // namespace ontask
