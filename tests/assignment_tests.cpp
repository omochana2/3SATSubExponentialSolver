#include "three_sat/assignment.hpp"
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>

TEST(AssignmentTest, InitializesAllValuesToFalse) {
    Assignment values(3);
    EXPECT_EQ(values.variable_count(), 3u);
    for (std::size_t i = 1; i <= 3; ++i) EXPECT_FALSE(values.get(i));
}

TEST(AssignmentTest, UpdatesBoundaryVariablesIndependently) {
    Assignment values(3);
    values.set(1, true);
    values.set(3, true);
    EXPECT_TRUE(values.get(1));
    EXPECT_FALSE(values.get(2));
    EXPECT_TRUE(values.get(3));
    values.set(1, false);
    EXPECT_FALSE(values.get(1));
    EXPECT_TRUE(values.get(3));
}

TEST(AssignmentTest, CopiesHaveIndependentValues) {
    Assignment values(3);
    values.set(3, true);
    auto copy = values;
    copy.set(3, false);
    EXPECT_TRUE(values.get(3));
    EXPECT_FALSE(copy.get(3));
}

TEST(AssignmentTest, RejectsInvalidIdsWithoutChangingValues) {
    Assignment values(3);
    values.set(3, true);
    for (auto id : {std::size_t{0}, std::size_t{4}, std::numeric_limits<std::size_t>::max()}) {
        SCOPED_TRACE(id);
        EXPECT_THROW(values.get(id), std::out_of_range);
        EXPECT_THROW(values.set(id, true), std::out_of_range);
    }
    EXPECT_FALSE(values.get(1));
    EXPECT_FALSE(values.get(2));
    EXPECT_TRUE(values.get(3));
}

TEST(AssignmentTest, SupportsEmptyAssignmentsButRejectsAccess) {
    Assignment empty(0);
    EXPECT_EQ(empty.variable_count(), 0u);
    EXPECT_THROW(empty.get(0), std::out_of_range);
    EXPECT_THROW(empty.set(1, true), std::out_of_range);
}
