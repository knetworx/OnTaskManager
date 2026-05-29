#pragma once

#include "ActivityRepository.h"
#include "CategoryRepository.h"
#include "Database.h"
#include "ForegroundTracker.h"
#include "IdleDetector.h"
#include "MainWindow.h"
#include "SampleRepository.h"
#include "TrayIcon.h"

#include <QObject>
#include <QTimer>

#include <filesystem>
#include <memory>

namespace ontask {

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(const std::filesystem::path& dataDir, QObject* parent = nullptr);

    int run();

private slots:
    void tick();

private:
    storage::Database db_;
    storage::ActivityRepository activities_;
    storage::CategoryRepository categories_;
    storage::SampleRepository sampleRepo_;
    tracker::ForegroundTracker foregroundTracker_;
    tracker::IdleDetector idleDetector_;

    std::unique_ptr<ui::MainWindow> window_;
    std::unique_ptr<ui::TrayIcon>   tray_;
    QTimer                          sampleTimer_;
};

} // namespace ontask
