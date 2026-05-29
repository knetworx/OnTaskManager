#include "Application.h"

#include <QApplication>

namespace ontask {

namespace {

constexpr int kSampleIntervalMs       = 15'000; // doc default
constexpr int kActivityTimeoutSeconds = 120;
constexpr bool kIdleTrackingEnabled   = true;

} // namespace

Application::Application(const std::filesystem::path& dataDir, QObject* parent)
    : QObject(parent),
      db_(dataDir / "ontask.db"),
      activities_(db_),
      categories_(db_),
      sampleRepo_(db_) {}

int Application::run() {
    window_ = std::make_unique<ui::MainWindow>(activities_, categories_, sampleRepo_);
    tray_   = std::make_unique<ui::TrayIcon>();
    if (!tray_->install(QStringLiteral("OnTaskManager"))) {
        // No tray on this system — still launch the window so the app is usable.
    }

    QObject::connect(tray_.get(), &ui::TrayIcon::openRequested, this, [this]() {
        window_->showNormal();
        window_->raise();
        window_->activateWindow();
    });
    QObject::connect(tray_.get(), &ui::TrayIcon::quitRequested, this, []() {
        QApplication::quit();
    });

    window_->show();

    sampleTimer_.setInterval(kSampleIntervalMs);
    QObject::connect(&sampleTimer_, &QTimer::timeout, this, &Application::tick);
    sampleTimer_.start();
    tick();

    return QApplication::exec();
}

void Application::tick() {
    const auto sample = foregroundTracker_.sample();
    const auto now = std::chrono::system_clock::now();
    const bool idle = kIdleTrackingEnabled &&
                      idleDetector_.idleTime().count() >= kActivityTimeoutSeconds;

    if (idle) {
        sampleRepo_.insertIdle(now);
    } else if (sample && !sample->path.empty()) {
        const auto leafId = activities_.resolvePath(sample->path);
        sampleRepo_.insertActivity(leafId, sample->timestamp);
    }

    window_->refreshData();
}

} // namespace ontask
