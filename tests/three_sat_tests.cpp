#include "three_sat/three_sat.hpp"
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>

TEST(ThreeSATTest, StoresClausesInInsertionOrder) {
    ThreeSAT formula(3);
    EXPECT_EQ(formula.variable_count(), 3u);
    EXPECT_TRUE(formula.clauses().empty());
    formula.add_clause({1, -2, 3});
    formula.add_clause({-1, 2, -3});
    ASSERT_EQ(formula.clauses().size(), 2u);
    EXPECT_EQ(formula.clauses()[0], (ThreeSAT::Clause{1, -2, 3}));
    EXPECT_EQ(formula.clauses()[1], (ThreeSAT::Clause{-1, 2, -3}));
}

TEST(ThreeSATTest, EvaluatesMixedSignsForEveryAssignment) {
    ThreeSAT formula(3);
    formula.add_clause({1, -2, 3});
    formula.add_clause({-1, 2, -3});
    // Fixed truth table; expected results are data, not another evaluator.
    struct Row { bool x1, x2, x3, expected; };
    const Row rows[] = {
        {false, false, false, true},
        {false, false, true, true},
        {false, true, false, false},
        {false, true, true, true},
        {true, false, false, true},
        {true, false, true, false},
        {true, true, false, true},
        {true, true, true, true}
    };
    for (const auto& row : rows) {
        SCOPED_TRACE(::testing::Message() << row.x1 << ',' << row.x2 << ',' << row.x3);
        Assignment values(3);
        values.set(1, row.x1); values.set(2, row.x2); values.set(3, row.x3);
        EXPECT_EQ(formula.is_satisfied_by(values), row.expected);
    }
}

TEST(ThreeSATTest, RejectsInvalidLiteralsWithoutMutation) {
    ThreeSAT formula(3);
    formula.add_clause({1, 2, 3});
    for (int bad : {0, 4, -4, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        SCOPED_TRACE(bad);
        EXPECT_THROW(formula.add_clause({1, 2, bad}), std::invalid_argument);
        ASSERT_EQ(formula.clauses().size(), 1u);
        EXPECT_EQ(formula.clauses()[0], (ThreeSAT::Clause{1, 2, 3}));
    }
}

TEST(ThreeSATTest, RejectsMismatchedAssignmentsAndExcessiveVariableCounts) {
    ThreeSAT formula(3);
    EXPECT_THROW(formula.is_satisfied_by(Assignment(2)), std::invalid_argument);
    EXPECT_THROW(formula.is_satisfied_by(Assignment(4)), std::invalid_argument);
    EXPECT_THROW(ThreeSAT(static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1),
                 std::invalid_argument);
}

TEST(ThreeSATTest, RepeatedLiteralsExpressUnitsAndContradictions) {
    ThreeSAT unit(1);
    unit.add_clause({1, 1, 1});
    Assignment one(1);
    EXPECT_FALSE(unit.is_satisfied_by(one));
    one.set(1, true);
    EXPECT_TRUE(unit.is_satisfied_by(one));
    unit.add_clause({-1, -1, -1});
    EXPECT_FALSE(unit.is_satisfied_by(one));
    one.set(1, false);
    EXPECT_FALSE(unit.is_satisfied_by(one));
}

TEST(ThreeSATTest, TautologyAcceptsBothValues) {
    ThreeSAT formula(1);
    formula.add_clause({1, -1, 1});
    Assignment one(1);
    EXPECT_TRUE(formula.is_satisfied_by(one));
    one.set(1, true);
    EXPECT_TRUE(formula.is_satisfied_by(one));
}

TEST(ThreeSATTest, EmptyConjunctionIsTrue) {
    EXPECT_TRUE(ThreeSAT(3).is_satisfied_by(Assignment(3)));
    ThreeSAT empty(0);
    EXPECT_TRUE(empty.is_satisfied_by(Assignment(0)));
    EXPECT_THROW(empty.add_clause({1, 1, 1}), std::invalid_argument);
}
