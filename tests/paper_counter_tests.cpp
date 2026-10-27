#include "three_sat/detail/paper_counter.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
using namespace three_sat::detail;

TEST(PaperCounterTest, RecoversKnownNonconstantTaylorJetsByInclusionExclusion) {
    // For this filled orbit the symbolic sum is
    // H(T)=1-12T+(77/2)T^2-33T^3+(169/16)T^4.
    // Fixed coefficients exercise correction inputs, group measurements and cancellation.
    const Cnf formula{4, {{1}, {4}, {3}, {2}}};
    const auto design = build_paper_design(formula);
    PaperCounterStats stats;
    const auto first = recover_taylor(formula, design, 1, 3, &stats);
    const auto second = recover_taylor(formula, design, 2, 3, &stats);
    EXPECT_EQ(first.coefficients, (std::vector<Rational>{Rational(81, 16), Rational(33, 4), Rational(23, 8)}));
    EXPECT_EQ(second.coefficients, (std::vector<Rational>{36, 84, 94}));
    EXPECT_EQ(hermite_at_zero({1, 2}, {first, second}, 4), 1);
    EXPECT_EQ(stats.restricted_sums, 66u); // 2 * (1 + 4*2 + 6*4)
    EXPECT_GT(stats.measurement_tuples, 0u);
    EXPECT_GT(stats.local_assignments, 0u);
    EXPECT_THROW(recover_taylor(formula, design, 0, 3), std::invalid_argument);
}

TEST(PaperCounterTest, CountsAllFourPositiveTriplesThroughNonconstantDesignAndInterpolation) {
    // Every triple is nonempty exactly when at least two variables are true: 6+4+1.
    const Cnf formula{4, {{1, 2, 3}, {1, 2, 4}, {1, 3, 4}, {2, 3, 4}}};
    PaperCounterStats stats;
    EXPECT_EQ(count_with_paper_pipeline(formula, &stats), 11);
    EXPECT_GT(stats.nonconstant_forms, 0u);
    EXPECT_GT(stats.restricted_sums, 2u);
    EXPECT_GT(stats.measurement_tuples, 0u);
    EXPECT_GT(stats.local_assignments, 0u);
    EXPECT_EQ(stats.interpolation_calls, 1u);
}

TEST(PaperCounterTest, HandlesNegativeLiteralsFreeVariablesAndEmptyClauses) {
    EXPECT_EQ(count_with_paper_pipeline(Cnf{3, {{-1, -2, -3}}}), 7);
    EXPECT_EQ(count_with_paper_pipeline(Cnf{4, {{1, 2, 3}, {-1, -2, -3}}}), 12);
    EXPECT_EQ(count_with_paper_pipeline(Cnf{2, {{1, 2}, {1, -2}, {-1, 2}, {-1, -2}}}), 0);
    EXPECT_EQ(count_with_paper_pipeline(Cnf{0, {}}), 1);
    EXPECT_EQ(count_with_paper_pipeline(Cnf{2, {}}), 4);
    EXPECT_EQ(count_with_paper_pipeline(Cnf{2, {{}}}), 0);
}

TEST(PaperCounterTest, SignedFixtureReachesNonconstantAlgebraicLeafWithoutCoreSplit) {
    const auto formula = normalize(Cnf{4, {{1, 2, 3}, {1, -2, 4}, {-1, 3, 4}, {2, -3, -4}}});
    ASSERT_FALSE(find_core_branch(formula, 2));
    PaperCounterStats stats;
    EXPECT_EQ(count_with_paper_pipeline(formula, &stats), 8);
    EXPECT_GT(stats.nonconstant_forms, 0u);
    EXPECT_GT(stats.restricted_sums, 2u);
    EXPECT_EQ(stats.interpolation_calls, 1u);
}

TEST(PaperCounterTest, MultipleCopiesCancelToZeroForAnUnsatisfiableFormula) {
    const auto formula = normalize(Cnf{3, {{1,2,3}, {1,2,-3}, {1,-2,3}, {1,-2,-3},
                                         {-1,2,3}, {-1,2,-3}, {-1,-2,3}, {-1,-2,-3}}});
    const auto design = build_paper_design(formula);
    ASSERT_GT(design.measurements.size(), 1u);
    PaperCounterStats stats;
    EXPECT_EQ(count_with_paper_pipeline(formula, &stats), 0);
    EXPECT_GT(stats.nonconstant_forms, 0u);
    EXPECT_GT(stats.restricted_sums, 2u);
    EXPECT_EQ(stats.interpolation_calls, 1u);
}
