#include "three_sat/three_sat_reader.hpp"
#include "three_sat/three_sat_naive_solver.hpp"
#include <gtest/gtest.h>
#include "three_sat/three_sat_subexponential_solver.hpp"
#include "three_sat/detail/paper_counter.hpp"
#include <memory>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

class SolverCaseFileTest : public ::testing::TestWithParam<int> {};

TEST_P(SolverCaseFileTest, MatchesExpectedAnswers) {
    const char* input_path = std::getenv("THREE_SAT_INPUT");
    const char* output_path = std::getenv("THREE_SAT_EXPECTED");
    ASSERT_NE(input_path, nullptr);
    ASSERT_NE(output_path, nullptr);
    SCOPED_TRACE(input_path);
    std::ifstream input(input_path), expected_file(output_path);
    ASSERT_TRUE(input.is_open());
    ASSERT_TRUE(expected_file.is_open());
    const auto formula = ThreeSATReader{}.read(input);
    std::unique_ptr<ThreeSatSolver> implementation;
    std::ostringstream actual;
    if (GetParam() >= 2) {
        // Bypass simplifying preprocessing so every fixed fixture exercises the complete
        // algebraic backend, even formulas the public solver can settle immediately.
        three_sat::detail::PaperCounterStats stats;
        const auto cnf = three_sat::detail::normalize(formula);
        three_sat::detail::DesignParameters parameters;
        if (GetParam() == 3) { parameters.point_count = 3; parameters.interval_length = 3; }
        if (GetParam() == 4) { parameters.groups = 3; parameters.balanced_schedules = true; }
        const auto design = three_sat::detail::build_paper_design(cnf, parameters);
        const auto count = three_sat::detail::count_with_paper_pipeline(cnf, design, &stats);
        actual << "count " << count << '\n';
        EXPECT_EQ(stats.interpolation_calls, 1u);
        EXPECT_GE(stats.restricted_sums, 2u);
    } else {
        if (GetParam() == 1) implementation = std::make_unique<ThreeSatSubexponentialSolver>(formula);
        else implementation = std::make_unique<ThreeSatNaiveSolver>(formula);
        const ThreeSatSolver& solver = *implementation;
        const auto count = solver.count();
        EXPECT_EQ(solver.count(), count); // A cached read must preserve the result.
        actual << "count " << count << '\n';
    }
    // getline normalizes platform line endings; all other output text is compared exactly.
    std::string expected, line;
    while (std::getline(expected_file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        expected += line + '\n';
    }
    EXPECT_EQ(actual.str(), expected);
}


INSTANTIATE_TEST_SUITE_P(Algorithms, SolverCaseFileTest, ::testing::Values(0, 1, 2, 3, 4),
    [](const ::testing::TestParamInfo<int>& info) {
        switch (info.param) {
        case 1: return "Subexponential";
        case 2: return "PaperBackend";
        case 3: return "QuadraticThreePoints";
        case 4: return "BalancedThreeGroups";
        default: return "Naive";
        }
    });
