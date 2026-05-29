#pragma once

#include <QObject>
#include <QSystemTrayIcon>

class QMenu;

namespace ontask::ui {

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(QObject* parent = nullptr);

    bool install(const QString& tooltip);

signals:
    void openRequested();
    void quitRequested();

private:
    QSystemTrayIcon* icon_ = nullptr;
    QMenu* menu_ = nullptr;
};

} // namespace ontask::ui
