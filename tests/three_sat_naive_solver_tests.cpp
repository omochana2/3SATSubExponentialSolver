#include "three_sat/three_sat_naive_solver.hpp"
#include <gtest/gtest.h>

TEST(ThreeSatNaiveSolverTest, OwnsFormulaSnapshot) {
    ThreeSAT formula(3);
    formula.add_clause({1, 2, 3});
    ThreeSatNaiveSolver snapshot(formula);
    formula.add_clause({-1, -2, -3});
    EXPECT_EQ(snapshot.count(), 7u);
    EXPECT_EQ(ThreeSatNaiveSolver(formula).count(), 6u);
}


TEST(ThreeSatNaiveSolverTest, OtherFormulaQueriesDoNotReplaceCachedCount) {
    ThreeSatNaiveSolver solver(ThreeSAT(3));
    EXPECT_EQ(solver.count(), 8u);
    ThreeSAT other(1);
    other.add_clause({1, 1, 1});
    EXPECT_EQ(solver.count(other), 1u);
    EXPECT_EQ(solver.count(), 8u);
}
