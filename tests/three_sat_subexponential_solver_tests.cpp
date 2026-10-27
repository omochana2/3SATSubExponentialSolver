#include "three_sat/three_sat_subexponential_solver.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

TEST(ThreeSatSubexponentialSolverTest, CountsThroughBaseInterface) {
    ThreeSAT formula(3);
    formula.add_clause({1, 2, 3});
    ThreeSatSubexponentialSolver implementation(formula);
    const ThreeSatSolver& solver = implementation;
    EXPECT_EQ(solver.count(), 7u);
    const auto witness = solver.get_satisfiable();
    ASSERT_TRUE(witness);
    EXPECT_FALSE(witness->get(1));
    EXPECT_FALSE(witness->get(2));
    EXPECT_TRUE(witness->get(3));
}

TEST(ThreeSatSubexponentialSolverTest, OtherFormulaQueriesDoNotReplaceCachedCount) {
    ThreeSatSubexponentialSolver solver(ThreeSAT(3));
    EXPECT_EQ(solver.count(), 8u);
    ThreeSAT other(1);
    other.add_clause({1, 1, 1});
    EXPECT_EQ(solver.count(other), 1u);
    EXPECT_EQ(solver.count(), 8u);
}

TEST(ThreeSatSubexponentialSolverTest, HandlesLargeFreeVariableCountsExactly) {
    EXPECT_EQ(ThreeSatSubexponentialSolver(ThreeSAT(63)).count(), std::uint64_t{1} << 63);
    EXPECT_THROW(ThreeSatSubexponentialSolver(ThreeSAT(64)).count(), std::overflow_error);
    ThreeSAT fixed(64);
    fixed.add_clause({1, 1, 1});
    EXPECT_EQ(ThreeSatSubexponentialSolver(fixed).count(), std::uint64_t{1} << 63);
    ThreeSAT product(65);
    product.add_clause({1, 2, 2});
    product.add_clause({-1, -2, -2});
    EXPECT_THROW(ThreeSatSubexponentialSolver(product).count(), std::overflow_error);
}

TEST(ThreeSatSubexponentialSolverTest, UnsatDominatesOverflowingFreeFactor) {
    // 68 unused variables must not cause overflow before this contradiction is evaluated.
    ThreeSAT formula(70);
    formula.add_clause({1, 2, 2});
    formula.add_clause({1, -2, -2});
    formula.add_clause({-1, 2, 2});
    formula.add_clause({-1, -2, -2});
    EXPECT_EQ(ThreeSatSubexponentialSolver(formula).count(), 0u);
}

TEST(ThreeSatSubexponentialSolverTest, LongForcedChainHasOneModel) {
    ThreeSAT formula(100);
    formula.add_clause({1, 1, 1});
    for (int i = 1; i < 100; ++i) formula.add_clause({-i, i + 1, i + 1});
    ThreeSatSubexponentialSolver solver(formula);
    EXPECT_EQ(solver.count(), 1u);
    const auto witness = solver.get_satisfiable();
    ASSERT_TRUE(witness);
    for (std::size_t i = 1; i <= 100; ++i) EXPECT_TRUE(witness->get(i));
}
