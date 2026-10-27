#include "three_sat/detail/paper_pipeline.hpp"
#include <gtest/gtest.h>
using namespace three_sat::detail;

TEST(PaperPipelineTest, SignedFormulaRunsEveryStageAndReportsSmallInputParameters) {
    const Cnf formula{4, {{1,2,3}, {1,-2,4}, {-1,3,4}, {2,-3,-4}}};
    PaperPipelineStats stats;
    EXPECT_EQ(count_paper_pipeline(formula, &stats), 8);
    EXPECT_EQ(stats.sparse_leaves, 1u);
    EXPECT_EQ(stats.occurrence_checks, 1u);
    EXPECT_EQ(stats.copy_reductions, 0u); // Already at most three occurrences per variable.
    EXPECT_EQ(stats.balanced_designs, 1u);
    EXPECT_EQ(stats.algebra.interpolation_calls, 1u);
    EXPECT_GT(stats.algebra.restricted_sums, 0u);
    EXPECT_TRUE(stats.small_input_sparsification);
    EXPECT_EQ(stats.sparsification_threshold, 2u);
    EXPECT_EQ(stats.small_input_leaves, 1u);
    EXPECT_EQ(stats.asymptotic_leaves, 0u);
    EXPECT_FALSE(stats.parameter_failures.empty());
}

TEST(PaperPipelineTest, SparseChildrenAreCountedExactlyOnceAndStatsReset) {
    PaperPipelineStats stats;
    EXPECT_EQ(count_paper_pipeline(Cnf{4, {{1,2,3}, {1,2,4}}}, &stats), 13);
    EXPECT_EQ(stats.sparse_leaves, 2u);
    EXPECT_EQ(stats.occurrence_checks, stats.sparse_leaves);
    EXPECT_EQ(stats.balanced_designs, stats.sparse_leaves);
    EXPECT_EQ(stats.algebra.interpolation_calls, stats.sparse_leaves);
    EXPECT_EQ(count_paper_pipeline(Cnf{0, {}}, &stats), 1);
    EXPECT_EQ(stats.sparse_leaves, 1u);
    EXPECT_EQ(stats.algebra.interpolation_calls, 1u);
    EXPECT_EQ(count_paper_pipeline(Cnf{0, {{}}}), 0);
}

TEST(PaperPipelineTest, OccurrenceStageIsIdentityOnlyWhenItsBoundAlreadyHolds) {
    const auto identity = ensure_bounded_occurrences(Cnf{3, {{1,2}, {-1,-2}}});
    EXPECT_EQ(identity.formula.variable_count, 3u);
    EXPECT_EQ(identity.copies, (std::vector<std::vector<int>>{{1}, {2}, {3}}));
    const auto copied = ensure_bounded_occurrences(Cnf{2, {{1,2}, {1,-2}, {-1,2}, {-1,-2}}});
    EXPECT_EQ(copied.formula.variable_count, 8u);
    std::vector<unsigned> degrees(9);
    for (const auto& clause : copied.formula.clauses) for (int literal : clause)
        EXPECT_LE(++degrees[literal < 0 ? -literal : literal], 3u);
}
