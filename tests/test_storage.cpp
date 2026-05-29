#include "ActivityRepository.h"
#include "CategoryRepository.h"
#include "Database.h"
#include "SampleRepository.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

namespace {

std::filesystem::path tempDbPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST(Storage, SchemaCreatesClean) {
    const auto path = tempDbPath("ontask_schema.db");
    std::filesystem::remove(path);
    {
        ontask::storage::Database db(path);
        (void)db;
    }
    std::filesystem::remove(path);
}

TEST(Storage, ResolvePathIsIdempotent) {
    const auto path = tempDbPath("ontask_resolve.db");
    std::filesystem::remove(path);
    {
        ontask::storage::Database db(path);
        ontask::storage::ActivityRepository repo(db);

        const auto a = repo.resolvePath({L"Chrome", L"Reddit", L"r/foo"});
        const auto b = repo.resolvePath({L"Chrome", L"Reddit", L"r/foo"});
        EXPECT_EQ(a, b);

        const auto c = repo.resolvePath({L"Chrome", L"Reddit", L"r/bar"});
        EXPECT_NE(a, c);
    }
    std::filesystem::remove(path);
}

TEST(Storage, EffectiveCategoryWalksAncestors) {
    const auto path = tempDbPath("ontask_effective.db");
    std::filesystem::remove(path);
    {
        ontask::storage::Database db(path);
        ontask::storage::ActivityRepository activities(db);
        ontask::storage::CategoryRepository categories(db);

        const auto work    = categories.createRoot(L"Work");
        const auto nonWork = categories.createRoot(L"Non-Work");

        const auto chrome   = activities.resolvePath({L"Chrome"});
        const auto facebook = activities.resolvePath({L"Chrome", L"Facebook"});
        const auto reddit   = activities.resolvePath({L"Chrome", L"Reddit"});
        const auto sub      = activities.resolvePath({L"Chrome", L"Reddit", L"r/foo"});

        // Map Chrome to Non-Work; Reddit subtree overrides nothing.
        activities.assignToCategory(chrome, nonWork);
        EXPECT_EQ(activities.effectiveCategoryOrZero(facebook), nonWork);
        EXPECT_EQ(activities.effectiveCategoryOrZero(sub), nonWork);

        // Override Reddit -> Work; descendants inherit the override.
        activities.assignToCategory(reddit, work);
        EXPECT_EQ(activities.effectiveCategoryOrZero(reddit), work);
        EXPECT_EQ(activities.effectiveCategoryOrZero(sub), work);
        EXPECT_EQ(activities.effectiveCategoryOrZero(facebook), nonWork);

        // Clearing an assignment falls back to ancestor.
        activities.clearAssignment(reddit);
        EXPECT_EQ(activities.effectiveCategoryOrZero(reddit), nonWork);
    }
    std::filesystem::remove(path);
}

TEST(Storage, SampleRepositoryRoundtrip) {
    const auto path = tempDbPath("ontask_samples.db");
    std::filesystem::remove(path);
    {
        ontask::storage::Database db(path);
        ontask::storage::ActivityRepository activities(db);
        ontask::storage::SampleRepository samples(db);

        const auto leaf = activities.resolvePath({L"Visual Studio", L"OnTaskManager", L"main.cpp"});

        const auto t0 = std::chrono::system_clock::now();
        const auto t1 = t0 + std::chrono::seconds(30);
        samples.insertActivity(leaf, t0);
        samples.insertIdle(t1);

        const auto rows = samples.samplesInRange(t0, t1 + std::chrono::seconds(1));
        ASSERT_EQ(rows.size(), 2u);
        EXPECT_FALSE(rows[0].isIdle);
        ASSERT_TRUE(rows[0].activityNodeId.has_value());
        EXPECT_EQ(*rows[0].activityNodeId, leaf);
        EXPECT_TRUE(rows[1].isIdle);
        EXPECT_FALSE(rows[1].activityNodeId.has_value());

        // Half-open: end bound excludes.
        const auto rowsExcl = samples.samplesInRange(t0, t1);
        EXPECT_EQ(rowsExcl.size(), 1u);
    }
    std::filesystem::remove(path);
}
