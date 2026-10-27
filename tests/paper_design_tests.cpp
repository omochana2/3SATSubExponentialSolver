#include "three_sat/detail/paper_design.hpp"
#include <gtest/gtest.h>
using namespace three_sat::detail;

TEST(PaperDesignTest, ReproducesRemarkFourPointFour) {
    // Fill exactly the orbit zeta_0, zeta_12, zeta_2, zeta_1 in that factor order.
    const auto design = build_paper_design(Cnf{4, {{1}, {4}, {3}, {2}}});
    ASSERT_EQ(design.measurements.size(), 1u);
    EXPECT_EQ(design.measurements[0], (std::vector<int>{1, -1, -1, 1}));
    const auto& form = design.forms[0][0];
    EXPECT_EQ(form.variable, 0u);
    EXPECT_EQ(form.coordinates, (std::vector<Polynomial>{{1, 0}, {0, Rational(-1, 2)}, {0, -1}, {0, 0}}));
    EXPECT_EQ(form.tail_sign, 1);
}

TEST(PaperDesignTest, EveryOffGroupCoefficientIsItsSharedMeasurementCombination) {
    const Cnf formula{5, {{1, 2, 3}, {-1, 4, 5}, {2, -3, 5}, {-2, -4, -5}, {1, 3, 4}, {1, 2}}};
    const auto design = build_paper_design(formula);
    for (int t : {1, 2}) for (std::size_t f = 0; f < formula.clauses.size(); ++f)
        for (const auto& form : design.forms[f]) for (std::size_t v = 0; v < formula.variable_count; ++v) {
            const unsigned bit = t == 1 ? 1 : 2;
            if ((design.variable_labels[v] & bit) == (design.factor_labels[f] & bit)) continue;
            const Rational actual = polynomial_value(form.coordinates[v], t);
            const Rational expected = form.tail_sign * design.measurements[form.copy][v];
            EXPECT_EQ(actual, expected) << "point=" << t << " factor=" << f << " variable=" << v;
        }
}

TEST(PaperDesignTest, UsesSeparateCopiesForRepeatedIncidenceLabels) {
    const auto design = build_paper_design(Cnf{1, {{1}, {1}, {1}, {1}, {1}}});
    EXPECT_EQ(design.measurements.size(), 2u);
    EXPECT_NE(design.forms[0][0].copy, design.forms[4][0].copy);
    for (const auto& factor : design.forms) EXPECT_EQ(factor[0].variable, 0u);
}
