#pragma once

#include "ActivityRepository.h"
#include "Database.h"
#include "ForegroundTracker.h"
#include "IdleDetector.h"
#include "MainWindow.h"
#include "TrayIcon.h"

#include <filesystem>

namespace ontask {

class Application {
public:
    explicit Application(const std::filesystem::path& dataDir);

    int run();

private:
    storage::Database db_;
    storage::ActivityRepository repo_;
    tracker::ForegroundTracker foregroundTracker_;
    tracker::IdleDetector idleDetector_;
    ui::TrayIcon tray_;
    ui::MainWindow window_;
};

} // namespace ontask
