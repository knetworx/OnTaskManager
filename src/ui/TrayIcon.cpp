#include "TrayIcon.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QStyle>

namespace ontask::ui {

TrayIcon::TrayIcon(QObject* parent) : QObject(parent) {}

bool TrayIcon::install(const QString& tooltip) {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }
    icon_ = new QSystemTrayIcon(this);
    icon_->setToolTip(tooltip);
    icon_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    menu_ = new QMenu();
    auto* openAct = menu_->addAction(tr("&Open OnTaskManager"));
    menu_->addSeparator();
    auto* quitAct = menu_->addAction(tr("&Quit"));
    icon_->setContextMenu(menu_);

    connect(openAct, &QAction::triggered, this, &TrayIcon::openRequested);
    connect(quitAct, &QAction::triggered, this, &TrayIcon::quitRequested);
    connect(icon_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger ||
                    reason == QSystemTrayIcon::DoubleClick) {
                    emit openRequested();
                }
            });

    icon_->show();
    return true;
}

} // namespace ontask::ui
