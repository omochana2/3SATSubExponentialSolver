#include "three_sat/detail/paper_counter.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
using namespace three_sat::detail;

TEST(PaperDesignValidationTest, CountsCubicFormsAtThreeArbitraryPointsAndThreeGroups) {
    // P(T)x=(1+T^3)x1. For the single positive unit factor, H(T)=4+4T^3.
    const Cnf formula{3, {{1}}};
    PaperDesign design;
    design.variable_count = 3;
    design.forms = {{{0, {{1, 0, 0, 1}, {}, {}}, 0, 0}}};
    for (int t : {-2, 3, 5}) {
        DesignPoint point{t, 3, {0, 1, 2}, {1}, {{{1, 0, 0}}, {}, {}}, {}};
        complete_design_point(design, point);
        design.points.push_back(std::move(point));
    }
    EXPECT_NO_THROW(validate_paper_design(formula, design));
    EXPECT_EQ(recover_taylor(formula, design, 3, 2).coefficients,
              (std::vector<Rational>{112, 108}));
    EXPECT_EQ(count_with_paper_pipeline(formula, design), 4);
}

TEST(PaperDesignValidationTest, RejectsBrokenIdentitiesBeforeEvaluating) {
    const Cnf formula{3, {{1, 2, 3}}};
    auto design = build_paper_design(formula);
    design.forms[0][0].coordinates[0][0] = 2;
    EXPECT_THROW(count_with_paper_pipeline(formula, design), std::invalid_argument);
    design = build_paper_design(formula);
    design.points[0].forms[0][0].local_coefficients[0] += 1;
    EXPECT_THROW(count_with_paper_pipeline(formula, design), std::invalid_argument);
    design = build_paper_design(formula);
    design.points.push_back(design.points[0]);
    EXPECT_THROW(count_with_paper_pipeline(formula, design), std::invalid_argument);
    design = build_paper_design(formula);
    design.points[0].variable_groups.pop_back();
    EXPECT_THROW(count_with_paper_pipeline(formula, design), std::invalid_argument);
}

TEST(PaperDesignValidationTest, RedundantIntegerMeasurementsAreRemovedExactly) {
    const Cnf formula{1, {{1}}};
    auto design = build_paper_design(formula);
    DesignPoint point{7, 3, {0}, {2}, {{{2}, {4}, {0}}, {}, {}}, {}};
    complete_design_point(design, point);
    ASSERT_EQ(point.measurements[0].size(), 1u);
    EXPECT_EQ(point.forms[0][0].measurement_weights[0][0], Rational(1, 2));
    design.points = {point};
    EXPECT_EQ(count_with_paper_pipeline(formula, design), 1);
    point.measurements[0].clear();
    EXPECT_THROW(complete_design_point(design, point), std::invalid_argument);
}
