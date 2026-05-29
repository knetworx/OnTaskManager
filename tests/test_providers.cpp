#include "providers/ChromeProvider.h"
#include "providers/GenericProvider.h"
#include "providers/VisualStudioProvider.h"

#include <gtest/gtest.h>

using ontask::tracker::ChromeProvider;
using ontask::tracker::GenericProvider;
using ontask::tracker::VisualStudioProvider;

TEST(GenericProvider, StripsExeAndCapitalises) {
    GenericProvider g;
    EXPECT_TRUE(g.matches(L"anything.exe"));
    const auto path = g.derivePath(L"notepad.exe", L"Untitled - Notepad");
    ASSERT_EQ(path.size(), 1u);
    EXPECT_EQ(path[0], L"Notepad");
}

TEST(GenericProvider, EmptyExeFallsBack) {
    GenericProvider g;
    const auto path = g.derivePath(L"", L"");
    ASSERT_EQ(path.size(), 1u);
    EXPECT_EQ(path[0], L"Unknown");
}

TEST(ChromeProvider, MatchesChromeAndEdge) {
    ChromeProvider c;
    EXPECT_TRUE(c.matches(L"chrome.exe"));
    EXPECT_TRUE(c.matches(L"Chrome.exe"));
    EXPECT_TRUE(c.matches(L"msedge.exe"));
    EXPECT_FALSE(c.matches(L"firefox.exe"));
}

TEST(ChromeProvider, ParsesPageDashSite) {
    ChromeProvider c;
    const auto path = c.derivePath(L"chrome.exe",
                                   L"PHX-123 - Jira - Google Chrome");
    ASSERT_GE(path.size(), 3u);
    EXPECT_EQ(path[0], L"Chrome");
    EXPECT_EQ(path[1], L"Jira");
    EXPECT_EQ(path[2], L"PHX-123");
}

TEST(ChromeProvider, ParsesRedditSubreddit) {
    ChromeProvider c;
    const auto path = c.derivePath(L"chrome.exe",
                                   L"Cool thread title : r/programming - Reddit - Google Chrome");
    ASSERT_GE(path.size(), 3u);
    EXPECT_EQ(path[0], L"Chrome");
    EXPECT_EQ(path[1], L"Reddit");
    EXPECT_EQ(path[2], L"r/programming");
}

TEST(ChromeProvider, FallsBackOnUnknownTitle) {
    ChromeProvider c;
    const auto path = c.derivePath(L"chrome.exe", L"Some Page");
    ASSERT_GE(path.size(), 2u);
    EXPECT_EQ(path[0], L"Chrome");
    EXPECT_EQ(path[1], L"Some Page");
}

TEST(VisualStudioProvider, MatchesDevenv) {
    VisualStudioProvider v;
    EXPECT_TRUE(v.matches(L"devenv.exe"));
    EXPECT_FALSE(v.matches(L"Code.exe"));
}

TEST(VisualStudioProvider, ParsesFileProject) {
    VisualStudioProvider v;
    const auto path = v.derivePath(
        L"devenv.exe",
        L"main.cpp (Working Tree) - OnTaskManager - Microsoft Visual Studio");
    ASSERT_GE(path.size(), 3u);
    EXPECT_EQ(path[0], L"Visual Studio");
    EXPECT_EQ(path[1], L"OnTaskManager");
    EXPECT_EQ(path[2], L"main.cpp");
}

TEST(VisualStudioProvider, FallsBackOnUnusualTitle) {
    VisualStudioProvider v;
    const auto path = v.derivePath(L"devenv.exe", L"Start Page - Microsoft Visual Studio");
    ASSERT_GE(path.size(), 2u);
    EXPECT_EQ(path[0], L"Visual Studio");
    EXPECT_EQ(path[1], L"Start Page");
}
