#include "Application.h"

#include <QApplication>
#include <QMessageBox>

#include <exception>
#include <filesystem>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OnTaskManager"));
    QApplication::setOrganizationName(QStringLiteral("OnTaskManager"));
    QApplication::setQuitOnLastWindowClosed(false);

    try {
        ontask::Application a(std::filesystem::current_path());
        return a.run();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, QStringLiteral("OnTaskManager"),
                              QString::fromUtf8(e.what()));
        return 1;
    }
}
