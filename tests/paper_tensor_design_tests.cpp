#include "three_sat/detail/paper_counter.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
using namespace three_sat::detail;

TEST(PaperTensorDesignTest, GeneralConstructionAgreesWithSpecializedRemark) {
    const Cnf formula{4, {{1}, {4}, {3}, {2}}};
    const auto specialized = build_paper_design(formula);
    const auto general = build_paper_design(formula, DesignParameters{});
    for (std::size_t f = 0; f < formula.clauses.size(); ++f)
        EXPECT_EQ(specialized.forms[f][0].coordinates, general.forms[f][0].coordinates);
    EXPECT_EQ(count_with_paper_pipeline(formula, general), 1);
}

TEST(PaperTensorDesignTest, QuadraticThreePointConstructionCountsUnsatExactly) {
    const auto formula = normalize(Cnf{3, {{1,2,3}, {1,2,-3}, {1,-2,3}, {1,-2,-3},
                                         {-1,2,3}, {-1,2,-3}, {-1,-2,3}, {-1,-2,-3}}});
    const auto design = build_paper_design(formula, DesignParameters{2, 3, 0, 3});
    std::size_t maximum_degree = 0;
    for (const auto& factor : design.forms) for (const auto& form : factor)
        for (const auto& p : form.coordinates) maximum_degree = std::max(maximum_degree, polynomial_degree(p));
    EXPECT_EQ(maximum_degree, 2u);
    EXPECT_EQ(design.points.size(), 3u);
    EXPECT_EQ(count_with_paper_pipeline(formula, design), 0);
}

TEST(PaperTensorDesignTest, ThreeGroupsAndShiftedSpectralIntervalCountSignedFormula) {
    const Cnf formula{4, {{1, 2, 3}, {1, -2, 4}, {-1, 3, 4}, {2, -3, -4}}};
    for (const auto parameters : {DesignParameters{3, 2, 0, 2}, DesignParameters{2, 2, 1, 2},
                                  DesignParameters{2, 3, 0, 2}}) {
        const auto design = build_paper_design(formula, parameters);
        EXPECT_EQ(design.points[0].group_count, parameters.groups);
        EXPECT_EQ(count_with_paper_pipeline(formula, design), 8);
    }
}

TEST(PaperTensorDesignTest, RejectsInvalidIntervalsAndExplicitResourceOverruns) {
    const Cnf formula{0, {}};
    EXPECT_THROW(build_paper_design(formula, DesignParameters{1, 2, 0, 2}), std::invalid_argument);
    EXPECT_THROW(build_paper_design(formula, DesignParameters{2, 2, 2, 2}), std::invalid_argument);
    EXPECT_THROW(build_paper_design(formula, DesignParameters{2, 4, 0, 2, 16}), std::length_error);
}
