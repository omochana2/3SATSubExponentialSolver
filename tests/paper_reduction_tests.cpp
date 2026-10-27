#include "three_sat/detail/paper_reduction.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <stdexcept>
using namespace three_sat::detail;

TEST(PaperReductionTest, SelectsExactSparsificationThresholdIncludingFactorTwo) {
    const auto full = select_sparsification_parameters(1);
    EXPECT_EQ(full.threshold, 1024u);
    EXPECT_EQ(full.depth_coefficient, 12292);
    EXPECT_EQ(full.occurrence_bound, 2099202);
    const auto half = select_sparsification_parameters(Rational(1, 2));
    EXPECT_EQ(half.threshold, 2048u);
    EXPECT_THROW(select_sparsification_parameters(0), std::invalid_argument);
    EXPECT_THROW(select_sparsification_parameters(2), std::invalid_argument);
}

TEST(PaperReductionTest, EachCopyHasAtMostThreeOccurrencesAndPreservesUnusedVariables) {
    const auto result = reduce_occurrences(Cnf{3, {{1, 2}, {1, -2}}});
    ASSERT_EQ(result.copies.size(), 3u);
    EXPECT_EQ(result.copies[0].size(), 2u);
    EXPECT_EQ(result.copies[1].size(), 2u);
    EXPECT_EQ(result.copies[2].size(), 1u);
    EXPECT_EQ(result.formula.variable_count, 5u);
    std::vector<unsigned> degrees(6);
    for (const auto& clause : result.formula.clauses) for (int literal : clause)
        ++degrees[literal < 0 ? -literal : literal];
    EXPECT_LE(*std::max_element(degrees.begin(), degrees.end()), 3u);
    EXPECT_EQ(degrees[result.copies[2][0]], 0u);
    // x1 is forced and x2,x3 are free: four models, with no factor from extra copies.
    EXPECT_EQ(count_with_paper_pipeline(result.formula), 4);
}

TEST(PaperReductionTest, DisjointLeavesAreStreamedAndTheirFixedCountsAddUp) {
    // (x1 OR x2 OR x3) AND (x1 OR x2 OR x4): 12 + 1 = 13.
    std::size_t leaves = 0;
    Integer total = 0;
    visit_sparse_leaves(Cnf{4, {{1, 2, 3}, {1, 2, 4}}}, 2, [&](const Cnf& leaf) {
        ++leaves;
        EXPECT_FALSE(find_core_branch(leaf, 2));
        total += count_with_paper_pipeline(leaf);
    });
    EXPECT_EQ(leaves, 2u);
    EXPECT_EQ(total, 13);
    EXPECT_THROW(visit_sparse_leaves(Cnf{0, {}}, 1, [](const Cnf&) {}), std::invalid_argument);
}

TEST(PaperReductionTest, FullReducedRouteCountsXorAndEmptyFormulas) {
    DesignParameters parameters;
    parameters.balanced_schedules = true;
    PaperCounterStats stats;
    const auto sparse = select_sparsification_parameters(Rational(1, 2));
    EXPECT_EQ(count_reduced_paper_pipeline(Cnf{2, {{1, 2}, {-1, -2}}}, sparse.threshold, parameters, &stats), 2);
    EXPECT_GT(stats.interpolation_calls, 0u);
    EXPECT_EQ(count_reduced_paper_pipeline(Cnf{2, {}}, 2, parameters), 4);
    EXPECT_EQ(count_reduced_paper_pipeline(Cnf{0, {{}}}, 2, parameters), 0);
}
