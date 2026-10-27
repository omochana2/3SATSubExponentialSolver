#include "three_sat/detail/paper_schedule.hpp"
#include "three_sat/detail/paper_counter.hpp"
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
using namespace three_sat::detail;

TEST(PaperScheduleTest, ConditionalExpectationMatchesHandComputedPotential) {
    const auto result = choose_balanced_schedules(Cnf{2, {{1, 2}}}, 2, 1);
    EXPECT_EQ(result.initial_potential, Rational(5, 6));
    EXPECT_EQ(result.final_potential, Rational(1, 6));
    EXPECT_EQ(result.maximum_label_load, 1u);
    EXPECT_EQ(result.maximum_group_size, 1u);
    EXPECT_EQ(result.variable_labels, (std::vector<unsigned>{0, 1}));
    EXPECT_FALSE(result.balance_hypotheses);
}

TEST(PaperScheduleTest, BalancedSchedulesFeedTheFullAlgebraicPipeline) {
    const Cnf formula{4, {{1, 2, 3}, {1, -2, 4}, {-1, 3, 4}, {2, -3, -4}}};
    const auto first = choose_balanced_schedules(formula, 2, 2);
    const auto second = choose_balanced_schedules(formula, 2, 2);
    EXPECT_EQ(first.variable_labels, second.variable_labels);
    EXPECT_EQ(first.factor_labels, second.factor_labels);
    EXPECT_LE(first.final_potential, first.initial_potential);
    EXPECT_LE(first.initial_potential, 2);
    DesignParameters parameters;
    parameters.balanced_schedules = true;
    const auto design = build_paper_design(formula, parameters);
    EXPECT_EQ(design.variable_labels, first.variable_labels);
    EXPECT_EQ(count_with_paper_pipeline(formula, design), 8);
}

TEST(PaperScheduleTest, EmptyScopesAndEmptyFormulaRemainWellDefined) {
    const auto empty = choose_balanced_schedules(Cnf{0, {{}}}, 2, 2);
    EXPECT_EQ(empty.initial_potential, 0);
    EXPECT_EQ(empty.final_potential, 0);
    EXPECT_EQ(empty.factor_labels, (std::vector<unsigned>{0}));
    EXPECT_THROW(choose_balanced_schedules(Cnf{0, {}}, 2, 4, 4), std::length_error);
}

TEST(PaperScheduleTest, AsymptoticSelectionReportsSmallDomainFailureHonestly) {
    EXPECT_FALSE(select_asymptotic_parameters(Cnf{3, {{1, 2, 3}}}).admissible);
    const auto result = select_asymptotic_parameters(Cnf{static_cast<std::size_t>(std::numeric_limits<int>::max()), {}});
    EXPECT_FALSE(result.admissible);
    EXPECT_FALSE(result.failures.empty());
    EXPECT_LT(result.parameters.groups, 2u);
    EXPECT_THROW(select_asymptotic_parameters(Cnf{20, {}}, 0), std::invalid_argument);
}

TEST(PaperScheduleTest, ChecksLoadBoundsWhenTheBalanceHypothesesActuallyHold) {
    // 48 disjoint binary clauses: n=M=96, b=1, w=2, r=2, Q=1, D=4.
    // Thus 2(b+w)D^2=96=M and 2Qr^2=8<=n.
    Cnf formula{96, {}};
    for (int v = 1; v <= 96; v += 2) formula.clauses.push_back({v, v + 1});
    const auto result = choose_balanced_schedules(formula, 2, 1);
    ASSERT_TRUE(result.balance_hypotheses);
    EXPECT_LE(result.maximum_label_load, 48u);
    EXPECT_LE(result.maximum_group_size, 96u);
    EXPECT_LE(result.final_potential, result.initial_potential);
    EXPECT_LE(result.initial_potential, 2);
}
