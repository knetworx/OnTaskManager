#include "IdleDetector.h"

#include <gtest/gtest.h>

TEST(IdleDetector, ReturnsNonNegativeDuration) {
    ontask::tracker::IdleDetector d;
    EXPECT_GE(d.idleTime().count(), 0);
}
