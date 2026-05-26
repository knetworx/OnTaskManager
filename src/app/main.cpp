#include "Application.h"

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
    try {
        ontask::Application app(std::filesystem::current_path());
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
