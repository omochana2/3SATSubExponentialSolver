#include "three_sat/detail/cnf.hpp"
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>

using three_sat::detail::Cnf;
using three_sat::detail::Clause;
using three_sat::detail::normalize;
using three_sat::detail::find_core_branch;
using three_sat::detail::split_core;

TEST(CnfTest, NormalizesWithoutLosingDeclaredVariables) {
    const auto result = normalize(Cnf{5, {{1, 1, 2}, {2, 1}, {1, 2, 3}, {-4, 4, 5}}});
    EXPECT_EQ(result.variable_count, 5u);
    EXPECT_EQ(result.clauses, (std::vector<Clause>{{1, 2}}));
}
TEST(CnfTest, EmptyClauseDominatesAndEmptyFormulaIsRetained) {
    EXPECT_EQ(normalize(Cnf{3, {{1}, {}, {2, 3}}}).clauses, (std::vector<Clause>{{}}));
    EXPECT_TRUE(normalize(Cnf{3, {}}).clauses.empty());
    EXPECT_TRUE(normalize(Cnf{3, {{-1, 1}}}).clauses.empty());
}
TEST(CnfTest, RejectsInvalidInternalRepresentations) {
    EXPECT_THROW(normalize(Cnf{3, {{0}}}), std::invalid_argument);
    EXPECT_THROW(normalize(Cnf{3, {{4}}}), std::invalid_argument);
    EXPECT_THROW(normalize(Cnf{3, {{std::numeric_limits<int>::min()}}}), std::invalid_argument);
    EXPECT_THROW(normalize(Cnf{4, {{1, 2, 3, 4}}}), std::invalid_argument);
    EXPECT_THROW(find_core_branch(Cnf{0, {}}, 1), std::invalid_argument);
}
TEST(CnfTest, ChoosesSmallestWidthThenLargestCore) {
    auto formula = normalize(Cnf{5, {{1, 2, 3}, {1, 2, 4}, {-1, 3}, {-1, 5}}});
    const auto branch = find_core_branch(formula, 2);
    ASSERT_TRUE(branch);
    EXPECT_EQ(branch->width, 2u);
    EXPECT_EQ(branch->core, (Clause{-1}));
    formula = normalize(Cnf{4, {{1, 2, 3}, {1, 2, 4}}});
    const auto pair = find_core_branch(formula, 2);
    ASSERT_TRUE(pair);
    EXPECT_EQ(pair->core, (Clause{1, 2}));
    EXPECT_FALSE(find_core_branch(formula, 3));
}
TEST(CnfTest, ProducesExactDisjointChildrenFromPaperRule) {
    const auto formula = normalize(Cnf{5, {{1, 2, 3}, {1, 2, 4}, {-3, 5}}});
    const auto children = split_core(formula, {{1, 2}, 3});
    EXPECT_EQ(children.first.variable_count, 5u);
    EXPECT_EQ(children.second.variable_count, 5u);
    EXPECT_EQ(children.first.clauses, (std::vector<Clause>{{-3, 5}, {1, 2}}));
    EXPECT_EQ(children.second.clauses, (std::vector<Clause>{{-2}, {-1}, {3}, {4}, {-3, 5}}));
    EXPECT_THROW(split_core(formula, {{5}, 3}), std::invalid_argument);
}
