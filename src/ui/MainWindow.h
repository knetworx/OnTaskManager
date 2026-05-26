#pragma once

namespace ontask::ui {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    void show();
    void hide();
};

} // namespace ontask::ui
