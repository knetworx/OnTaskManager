#include "ActivityRepository.h"
#include "Database.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

TEST(Storage, OpensAndInsertsSample) {
    const auto path = std::filesystem::temp_directory_path() / "ontask_test.db";
    std::filesystem::remove(path);

    ontask::storage::Database db(path);
    ontask::storage::ActivityRepository repo(db);

    ontask::tracker::ActivitySample s;
    s.processName = L"test.exe";
    s.windowTitle = L"unit test";
    s.timestamp = std::chrono::system_clock::now();

    EXPECT_NO_THROW(repo.insert(s, std::chrono::seconds(0)));

    std::filesystem::remove(path);
}
